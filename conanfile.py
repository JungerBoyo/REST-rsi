from conan import ConanFile
from conan.tools.cmake import CMakeToolchain

class ChangeMeRecipe(ConanFile):
    build_policy = "missing"

    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps"

    def config_options(self):
        pass
        # options example 
        #   self.options["glad"].spec = "gl"
        #   self.options["glad"].extensions = "GL_ARB_gl_spirv"
        #   self.options["glad"].gl_profile = "core"
        #   self.options["glad"].gl_version = "4.5"

    def requirements(self):
        self.requires("pistache/cci.20240107")
        self.requires("spdlog/1.13.0")
        self.requires("nlohmann_json/3.11.3")
        # self.requires("glad/0.1.36")
        # conditional requires (it happens too often)
        # if (self.settings.os != 'Windows'): 
        #     self.requires("...")
