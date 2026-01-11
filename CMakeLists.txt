cmake_minimum_required(VERSION 3.14)
project(Hermes C)

# --- 1. Dependencies ---

# NATS: Built from submodule
set(NATS_BUILD_STREAMING OFF CACHE BOOL "" FORCE)
set(NATS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(NATS_BUILD_LIB_STATIC ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/nats)

# Protobuf-C: Found on system
find_package(PkgConfig REQUIRED)
pkg_check_modules(PROTOBUF_C REQUIRED libprotobuf-c)

# --- 2. Protobuf Generation ---
find_program(PROTOC_C protoc-c REQUIRED)

function(hermes_generate_proto PROTO_FILE OUTPUT_VAR)
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    set(GEN_C "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb-c.c")
    set(GEN_H "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_NAME}.pb-c.h")
    
    add_custom_command(
        OUTPUT "${GEN_C}" "${GEN_H}"
        COMMAND ${PROTOC_C} --c_out=${CMAKE_CURRENT_BINARY_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR}/proto ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        COMMENT "Generating Protobuf-C files..."
    )
    set(${OUTPUT_VAR} "${GEN_C}" "${GEN_H}" PARENT_SCOPE)
endfunction()

# --- 3. Hermes Library ---
add_library(hermes INTERFACE)

target_include_directories(hermes INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/nats/src>
    ${PROTOBUF_C_INCLUDE_DIRS}
)

# Link against nats_static and the system protobuf-c
target_link_libraries(hermes INTERFACE nats_static ${PROTOBUF_C_LIBRARIES})

# ---------------- Build Docs -------------------
find_program(PDFLATEX_COMPILER pdflatex)
find_program(BIBER_COMPILER biber)

if(PDFLATEX_COMPILER AND BIBER_COMPILER)
    set(LATEX_DOC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs")
    set(LATEX_PAPER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs/paper")
    set(PDF_OUTPUT "${LATEX_DOC_DIR}/hermes_paper.pdf")

    add_custom_command(
        OUTPUT "${PDF_OUTPUT}"
        # 1. Initial pass to create .bcf and .aux
        COMMAND ${PDFLATEX_COMPILER} -interaction=nonstopmode -output-directory=${LATEX_DOC_DIR} ${LATEX_PAPER_DIR}/hermes_paper.tex
        
        # 2. Run BIBER inside the docs directory (crucial for Arch security)
        COMMAND ${BIBER_COMPILER} hermes_paper
        
        # 3. Final passes to link bibliography and fix labels
        COMMAND ${PDFLATEX_COMPILER} -interaction=nonstopmode -output-directory=${LATEX_DOC_DIR} ${LATEX_PAPER_DIR}/hermes_paper.tex
        COMMAND ${PDFLATEX_COMPILER} -interaction=nonstopmode -output-directory=${LATEX_DOC_DIR} ${LATEX_PAPER_DIR}/hermes_paper.tex
        
        # 4. Artifact Cleanup: Remove everything except the PDF
        COMMAND ${CMAKE_COMMAND} -E remove 
                "${LATEX_DOC_DIR}/hermes_paper.aux" 
                "${LATEX_DOC_DIR}/hermes_paper.bcf" 
                "${LATEX_DOC_DIR}/hermes_paper.run.xml" 
                "${LATEX_DOC_DIR}/hermes_paper.bbl" 
                "${LATEX_DOC_DIR}/hermes_paper.blg" 
                "${LATEX_DOC_DIR}/hermes_paper.log" 
                "${LATEX_DOC_DIR}/hermes_paper.out" 
                "${LATEX_DOC_DIR}/hermes_paper.toc"
        
        WORKING_DIRECTORY "${LATEX_DOC_DIR}"
        DEPENDS "${LATEX_PAPER_DIR}/hermes_paper.tex" "${LATEX_PAPER_DIR}/references.bib"
        COMMENT "Generating Research Paper and cleaning artifacts..."
    )

    add_custom_target(docs ALL DEPENDS "${PDF_OUTPUT}")
endif()

# --- 4. Benchmark Example ---
set(heartbeat_proto "${CMAKE_CURRENT_SOURCE_DIR}/proto/heartbeat.proto")
hermes_generate_proto(${heartbeat_proto} PROTO_SOURCES)

# Publisher
add_executable(hermes_pub examples/publisher.c ${PROTO_SOURCES})
target_link_libraries(hermes_pub PRIVATE hermes)
target_compile_definitions(hermes_pub PRIVATE HERMES_IMPLEMENTATION)

# Subscriber
add_executable(hermes_sub examples/subscriber.c ${PROTO_SOURCES})
target_link_libraries(hermes_sub PRIVATE hermes)
target_link_libraries(hermes_sub PRIVATE m)
target_compile_definitions(hermes_sub PRIVATE HERMES_IMPLEMENTATION)
