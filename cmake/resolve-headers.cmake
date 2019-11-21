set(INCLUDES_COPIES "${CMAKE_CURRENT_SOURCE_DIR}/../include")

foreach(f IN ITEMS ${HEADERS})
    file(COPY ${f} DESTINATION "${INCLUDES_COPIES}/Helpz")
endforeach()

target_include_directories(${TARGET} PUBLIC ${INCLUDES_COPIES})
