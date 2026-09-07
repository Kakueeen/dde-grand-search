project(markdown-preview)

# 定义插件名称
set(LIB_NAME markdown-preview-plugin)

set(SRCS
    markdownpreview_global.h
    markdownpreviewinterface.h
    markdownpreviewinterface.cpp
    markdownpreviewplugin.h
    markdownpreviewplugin.cpp
    markdownview.h
    markdownview.cpp
)

add_library(${LIB_NAME} SHARED ${SRCS} ${QRCS})

target_link_libraries(${LIB_NAME} PRIVATE
    ${Qt_LIBS}
    ${DTK_LIBS}
)

# bin
install(TARGETS ${LIB_NAME} DESTINATION ${PLUGIN_PREVIEW_DIR})

# conf
install(FILES markdown-preview-plugin.conf DESTINATION ${PLUGIN_PREVIEW_DIR})
