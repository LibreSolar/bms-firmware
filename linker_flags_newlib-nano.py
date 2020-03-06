# Custom settings, as referred to as "extra_script" in platformio.ini
#
# See http://docs.platformio.org/en/latest/projectconf.html#extra-script
# and https://docs.platformio.org/en/latest/projectconf/advanced_scripting.html#extra-linker-flags-without-wl-prefix
#
# We are setting the required linker flags for use of newlib-nano


Import("env")

env.Append(
            LINKFLAGS=[
                "-Wl,-u,_printf_float",
                "--specs=nano.specs",
                "--specs=nosys.specs"
            ])

env.Append( CXXFLAGS=[ "-Wno-register" ] )
