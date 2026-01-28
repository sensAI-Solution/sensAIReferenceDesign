import cv2
import numpy as np
import os

import ml_engine_simulator
import ml_engine_simulator.network as ml_sim


print(f"Generating embedding vectors by using ML-Engine Simulator {ml_engine_simulator.__version__}")


# To be changed by user
# ====================
# Input data:
MODEL_FILE_PATH = r"C:\Users\user\Downloads\model-shapes\model-shapes\model-sensai-h5-best.h5"
REF_IMAGE_PATH = r"C:\Users\user\Downloads\model-shapes\model-shapes\img_ref_fixed.png"
THRESHOLD = 0.2

# Output data:
OUTPUT_DIR = r"C:\Users\user\Downloads\model-shapes\model-shapes"
EMBEDDING_BIN_FILE = r"defect_detection_ok_vitamins_embedding_vector_64_bytes.bin"
# ====================


def process_image(idx, image_path,
                  nn_input_size=(288, 384),
                  quant_value=128.0
                  ):
    """
    Read the image and process it to the train format.
    Args:
        nn_input_size: model input size
        quant_value: quantization value
    Returns:
        ndarray: image for the inference step with adding batch size shape
    """
    image = cv2.imread(image_path)
    image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
    image = cv2.resize(image, nn_input_size[::-1], interpolation=cv2.INTER_LINEAR)
    image = image/quant_value
    image = image.astype(np.float32)
    return image


def get_ref_images(image_path):
    reference_images = []
    if os.path.isfile(image_path):
        reference_images.append(image_path)
    elif os.path.isdir(image_path):
        for file in os.listdir(image_path):
            reference_images.append(os.path.join(image_path, file))

    processed_images = []
    for idx, image in enumerate(reference_images):
        processed_images.append(process_image(idx, image))

    return processed_images


def get_embedding_vectors(sim_result, nb_vectors):
    embedding_vectors = []
    for i in range(nb_vectors):
        vector = sim_result.get_network_output(i, clip=False)
        vector = vector * 1024
        if np.any((vector < -32768) | (vector > 32767)):
            print(f"Warning: The embedding vector #{i} has values outside the int16 range.")
        vector = vector.astype(np.int16)
        embedding_vectors.append(vector)
    return embedding_vectors


def save_embedding_vectors(embedding_vectors, embedding_vector_dir):
    dir_path = os.path.dirname(embedding_vector_dir)
    if dir_path and not os.path.exists(dir_path):
        os.makedirs(dir_path)
    file_path = os.path.join(embedding_vector_dir, EMBEDDING_BIN_FILE)
    with open(file_path, 'wb') as f:
        for vector in embedding_vectors:
            vector_bytes = vector.astype('<i2').tobytes()   # little-endian int16
            f.write(vector_bytes)
        threshold_bytes = np.array([THRESHOLD*1024], dtype='<i4').tobytes()
        f.write(threshold_bytes)
    print(f"{len(embedding_vectors)} embedding vector(s) saved to {file_path}")


simulator = ml_sim.FixedPointNetwork(
    network=MODEL_FILE_PATH,
    quant_path=ml_sim.get_default_quant_file(),
    input_types=["FPGA"]
)

inputs = get_ref_images(REF_IMAGE_PATH)
result = simulator.run(data=inputs, layerByLayer=False)
embedding_vectors = get_embedding_vectors(result, len(inputs))
save_embedding_vectors(embedding_vectors, OUTPUT_DIR)

