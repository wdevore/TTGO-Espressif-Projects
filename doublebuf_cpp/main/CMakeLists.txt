set(SOURCES
    doublebuf.cpp
    display.cpp
)

idf_component_register(
    SRCS ${SOURCES}
    REQUIRES
        tft
    INCLUDE_DIRS 
       "." 
       includes
)
