if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT OR NOT DEFINED FILTER_PATH)
  message(FATAL_ERROR
    "INPUT, OUTPUT, and FILTER_PATH must be specified")
endif()

file(READ "${INPUT}" CONTENT)

# macOS installs the filter outside CUPS' normal filter directory.
string(REPLACE
  "Filter application/vnd.cups-raster 33 rastertobrlaser"
  "Filter application/vnd.cups-raster 33 ${FILTER_PATH}"
  CONTENT
  "${CONTENT}")

# DCP and MFC models have scanners and should expose the Image Capture
# Architecture driver to macOS.
string(REGEX REPLACE
  "([ \\t]*ModelName \"(DCP|MFC)-[A-Z0-9-]+( series)?\")"
  "\\1\n  Attribute \"APICADriver\" \"\" \"True\""
  CONTENT
  "${CONTENT}")

file(WRITE "${OUTPUT}" "${CONTENT}")
