package("struct")
    set_homepage("https://github.com/svperbeast/struct")
    set_description("pack and unpack binary data.")
    set_license("MIT-License")

    add_urls("https://github.com/svperbeast/struct.git")
    add_deps("cmake")
    on_install(function (package)
        --import("package.tools.cmake").build(package, {"-DSTRUCT_BUILD_TEST=OFF"}, {buildir = "build"})
        --os.cp("build/Release/*", package:installdir("lib"))
        --os.cp("include/*", package:installdir("include"))
        import("package.tools.cmake").install(package, {"-DSTRUCT_BUILD_TEST=OFF"})
        os.cp(path.join(package:buildir(),"Release/include"), package:installdir())
        os.cp(path.join(package:buildir(),"Release/lib"), package:installdir())
        os.cp("LICENSE", package:installdir())
    end)

    on_test(function (package)
        assert(package:has_cincludes("struct/struct.h"))
        assert(package:has_cfuncs("struct_pack", {includes = "struct/struct.h"}))
        assert(package:has_cfuncs("struct_unpack", {includes = "struct/struct.h"}))
    end)
package_end()


package("sdl_fontcache")
    set_homepage("https://github.com/grimfang4/SDL_FontCache")
    set_description("A generic font caching C library with loading and rendering support for SDL.")
    set_license("MIT-License")

    add_urls("https://github.com/hugoc7/SDL_FontCache.git")
    add_deps("libsdl", "libsdl_ttf")
    add_includedirs("include", "include/SDL2")
    add_links("libsdl_fontcache")
    on_install(function (package)
       import("package.tools.xmake").install(package)
    end)

    on_test(function (package)
        assert(package:has_cincludes("SDL_FontCache.h"))
        assert(package:has_cfuncs("FC_CreateFont", {includes = "SDL_FontCache.h"}))
    end)
package_end()


package("CRCpp")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/d-bahr/CRCpp")
    set_description("Easy to use and fast C++ CRC library.")
    set_license("BSD")

    set_urls("https://github.com/d-bahr/CRCpp/archive/refs/tags/release-${version}.tar.gz",
             "https://github.com/d-bahr/CRCpp/archive/refs/tags/release-${version}.zip",
             "https://github.com/d-bahr/CRCpp.git")

    add_versions("1.1.0.0", "24c3eaf40291e3d3ac4f2d93f71cc7be261c325a")
    add_versions("1.0.1.0", "51fbc35ef892e98abe91a51f7320749c929d72bd")
    add_versions("1.0.0.0", "534c1d8c5517cfbb0a0f1ff0d9ec4c8b8ffd78e2")
    add_versions("0.2.0.6", "12df3a15e89aaa3a4b44bb357b15e1f7e20a2608")

    on_install(function (package)
        os.cp("inc/*.h", package:installdir("include"))
        os.cp("LICENSE", package:installdir())
    end)

    on_test(function (package)
        assert(package:has_cxxincludes("CRC.h"))
    end)
package_end()


add_requires("libsdl", "libsdl_ttf", "libsdl_net", "readerwriterqueue", "struct", "sdl_fontcache", "CRCpp")

target("Foster")
    set_kind("binary")
    add_files("$(projectdir)/src/*.cpp|Components.cpp")
    add_includedirs("$(projectdir)/include")
    add_headerfiles("$(projectdir)/include/**")
    set_rundir("$(projectdir)")
    add_packages("libsdl", "libsdl_ttf", "libsdl_net", "readerwriterqueue", "struct", "sdl_fontcache", "CRCpp")