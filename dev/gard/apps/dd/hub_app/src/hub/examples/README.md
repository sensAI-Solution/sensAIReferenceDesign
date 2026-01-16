# HUB Examples

### Code project of using a HUB installation

- Compile HUB code from a git clone
  - make build_hub
- Create a HUB .deb package
  - make package
- Install the .deb package
  - sudo apt install lscc-hub_\<version>_\<arch>.deb
  - Example: sudo apt install lscc-hub_1.0.0_amr64.deb
- Compile the given examples - you may need root perms since it is in /opt
  - cd c_example; make; ./a.out
  - cd cpp_example; make; ./a.out
- Note the CFLAGS and LDFLAGS variables for HUB and associated libraries in the given Makefile
- To use python library, 
  - Create a virtual environment and activate
  - Install hub module library using .whl package found at build/output/package directory
  - Example: `pip install lscc_hub-1.0.0-py3-none-manylinux_2_28_arm64.whl`
  - Run example using `cd py_example; python example-01.py`
