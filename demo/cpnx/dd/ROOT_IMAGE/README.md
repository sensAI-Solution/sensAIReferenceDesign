Folder content:
* shapes contains firmware trained with shape test patterns in this file [TestShapes-letter.pdf](../docs/TestShapes-letter.pdf)
* pills contains firmware trained with shape test pattern to be used with [TestShapes-letter.pdf](../docs/TestShapes-letter.pdf) and [TestShapes-letter.pdf](../docs/TestShapes-letter.pdf)
* generate_embedding_vector_short.py is the script to use to generate the embeding vector.

Note:
* Root images in these folders were generated using [RootImageBuilder.py](/dev/gard/platform_fw/src/scripts/RootImageBuilder.py). Please follow steps in the user guide in section _Compiling and Programming for FPGA_ to regenerate them.
* If you intend to train your own model you will need access to the defect detection backbone part of the Model Zoo for which you need to request access using this [link](https://github.com/sensAI-Solution/sensAIStudio/blob/main/request_access.md).
* Note that if you intend to retrain the defect detection model, you will also need to generate an embeding vector using the scipt in this directory. 
