# We use python to extract git version info and generate some other files,
# but it may be installed locally. The expected python version should be
# kept in sync with make/tools.mk.
PYTHON_DIR = qt-5.4.1/Tools/mingw491_32/opt/bin

# Search the python using environment override first
OPENPILOT_TOOLS_DIR = $$(OPENPILOT_TOOLS_DIR)
!isEmpty(OPENPILOT_TOOLS_DIR):exists($$OPENPILOT_TOOLS_DIR/$$PYTHON_DIR/python*) {
    PYTHON = \"$$OPENPILOT_TOOLS_DIR/$$PYTHON_DIR/python\"
} else {
    # If not found, search the predefined tools path
    exists($$ROOT_DIR/tools/$$PYTHON_DIR/python*) {
        PYTHON = \"$$ROOT_DIR/tools/$$PYTHON_DIR/python\"
    } else {
        # not found, hope it's in the path...
        PYTHON_VER = "$$system(python --version 2>&1)"
        contains(PYTHON_VER, "Python 2.*") {
            PYTHON = \"python\"
        } else {
            PYTHON = \"python2\"
        }
    }
}

PYTHON = $$replace(PYTHON, \\\\, /)
message(Using python interpreter: $$PYTHON)
