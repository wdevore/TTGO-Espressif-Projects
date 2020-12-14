set(SOURCES
    drawline.c
)

idf_component_register(
    SRCS ${SOURCES}
    REQUIRES
        tft
    INCLUDE_DIRS 
       "." 
       includes
)
