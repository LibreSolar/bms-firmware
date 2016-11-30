from SCons.Script import DefaultEnvironment
from os.path import join

env = DefaultEnvironment()

env.Replace(
#	UPLOADER=join("$PIOPACKAGES_DIR", "tool-stlink", "st-flash"),
    MYUPLOADERFLAGS=[
			"--reset",		# reset board before and after flashing
            "write",        # write in flash
            "$SOURCES",     # firmware path to flash
            "0x08000000"    # flash start adress
        ],
    UPLOADCMD='$UPLOADER $MYUPLOADERFLAGS',
)

#print env.Dump()
#print ARGUMENTS
