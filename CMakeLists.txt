# Definições de Copyright e Licença
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

set(PROJECT_NAME STLAB)

project(${PROJECT_NAME}
  VERSION 1.0.0
  LANGUAGES C CXX ASM
)

# NORDIC SDK APP START
# Adicionando fontes
target_sources(app PRIVATE app/main/main.c)
target_sources(app PRIVATE app/mqtt/mqtt.c)
target_sources(app PRIVATE app/lte/lte.c)

# Adicionando diretórios de inclusão
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app/main
    ${CMAKE_CURRENT_SOURCE_DIR}/app/mqtt
    ${CMAKE_CURRENT_SOURCE_DIR}/app/lte
)
# NORDIC SDK APP END
