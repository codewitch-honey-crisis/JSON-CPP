#
# This is a small tool to read files from the PlatformIO data directory and write
# them to a flashable BIN image in the LittleFS file format
#
# This script requires the LittleFS-Python mopackage
# https://pypi.org/project/littlefs-python/
#
#
# Author: Guido Henkel
# Date: 1/2/2020
#
import getopt, sys
from littlefs import LittleFS
from pathlib import Path


print( "Building LittleFS image…" )

argList = sys.argv[1:]
arxx = { argList[i]: argList[i+1] for i in range(0, len(argList)-1, 2) }
print( arxx )

dataPath = arxx["-c"]                                                               # Grab the source path parameter

fileList = [pth for pth in Path(dataPath).iterdir() if pth.stem[0] != '.']          # Create a list of all the files in the directory

fs = LittleFS(block_size=4096, block_count=3072)                                    # Open LittleFS, create a 12MB partition

for curFile in fileList:
    print( "Adding " + curFile.name )
    with open( curFile, 'rb' ) as f:                                                # Read the file into a buffer
        data = f.read()

    with fs.open( curFile.name, 'w') as fh:                                         # Write the file to the LittleFS partition
        fh.write( data )

outName = argList[-1]                                                               # The last argument is the destination file name
with open(outName, 'wb') as fh:                                                     # Dump the filesystem content to a file
    fh.write(fs.context.buffer)