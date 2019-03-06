message(STATUS "* Configuring translations")
include(Gettext)

#find_package(LibArchive REQUIRED)

GETTEXT_CREATE_TRANSLATIONS(pcsxr.pot ALL es fr hu it ko_KR pt_BR ru zh_CN zh_TW)
