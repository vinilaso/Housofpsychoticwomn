cmake_minimum_required(VERSION 3.21)
project(Housofpsychoticwomn LANGUAGES CXX)

# --- Padrão do C++ ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Qt --------------------------------------------------------------------
# Qt 6.8+ é necessário para o login com PKCE (sem client secret)
find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Network NetworkAuth)
qt_standard_project_setup()

# --- Executável ------------------------------------------------------------
qt_add_executable(${PROJECT_NAME}
    src/main.cpp
    src/network/http/HttpClient.h
    src/network/http/HttpClient.cpp
    src/network/http/HttpMethod.h
    src/network/http/HttpMethod.cpp
    src/network/http/HttpRequest.h
    src/network/http/HttpRequest.cpp
    src/network/ImageLoader.h
    src/network/ImageLoader.cpp
    src/network/UrlPath.h
    src/network/UrlPath.cpp
    src/security/Security.h
    src/security/Security.cpp
    src/spotify/SpotifyClient.h
    src/spotify/SpotifyClient.cpp
    src/spotify/TokenStore.h
    src/spotify/TokenStore.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/TopItemsDialog.h
    src/ui/TopItemsDialog.cpp
    src/ui/widgets/SmoothWheelScroll.h
    src/ui/widgets/SmoothWheelScroll.cpp
)

# Janela sem console no Windows
set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)

# --- Avisos do compilador --------------------------------------------------
if(MSVC)
    # /utf-8: lê o código-fonte como UTF-8 (acentos nos textos)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Network Qt6::NetworkAuth)

# --- DLLs do Qt ------------------------------------------------------------
# Copia as DLLs do Qt para a pasta do executável depois de cada compilação,
# para o app rodar direto do Visual Studio (ou com duplo clique no .exe).
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE crypt32)

    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}" REQUIRED)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime
                "$<TARGET_FILE:${PROJECT_NAME}>"
        COMMENT "Copiando as DLLs do Qt para a pasta do executável"
    )
endif()