import os
import glob


debug = ARGUMENTS.get('debug', '0')
target = ARGUMENTS.get('target', '0')

def normalizeLineEnding(fdir):
    # Add trailing / to path
    return fdir if fdir[-1] == os.sep else fdir+os.sep

def getSrcFromFolder(srcDirs, srcPattern, trgtDir):
    print("Extract files from " , srcDirs)
    res=[]
    trgtDir = normalizeLineEnding(trgtDir)
    for srcF in srcDirs:
        srcF = normalizeLineEnding(srcF)
        print("Process %s folder with pattern \"%s\"" % (srcF,srcF+srcPattern))
        #print(glob.glob(srcF+srcPattern))
        for f in glob.glob(srcF+srcPattern):
            print("  Append %s" %(trgtDir+f)) 
            res.append(trgtDir+f)
    return tuple(res)

if debug:
    print("*** Debug build...")
    binFolder = './'
else:
    print("*** Release build...")
    binFolder = './'
# Google stuff
googletest_framework_root = "./test/googletest"

googletest_include_paths = (
    googletest_framework_root + "/googletest/include",
    googletest_framework_root + "/googlemock/include")


testComFiles =()
testComLib = 'ctest'
cutLib  =   'cut'
incPath=()
linkLibs =()
cutFolders =()
testCutFolders = ()
genTestFolders = ('./test/',)

if  target == 'test_common':
    print("Create common tests.")
    cutFolders += ('./common/src/',)
    testCutFolders = ('./common/test/',)
    ccFlags  = '-DUNIT_TEST '
    incPath +=('./common/inc/',)
    incPath +=('./common/test/',)
    incPath += ('./test/',)
    incPath+= (googletest_include_paths[0],)
    incPath+= (googletest_include_paths[1],)
    linkLibs +=('pthread',)   
    linkFlags = '-Xlinker -Map=output.map'
    debug = True
elif target == 'test_display':
    print("Create display tests.")
    cutFolders += ('./display/src/', )
    testCutFolders = ('./display/test/',)
    testComFiles += getSrcFromFolder(genTestFolders,'mock_common.cpp',binFolder)
    incPath+=('display/inc/',)
    ccFlags  = '-DUNIT_TEST '
elif target == 'test_pin':
    print("Create pin tests.")
    #cutFolders += ('./pin/drv/mock/', )
    #cutFolders += ('./pin/drv/stm32/f4/', )
    testCutFolders = ('./pin/test/',)
    incPath+=(googletest_include_paths,)
    incPath+=('pin/inc/',)
    incPath+=('pin/test/',)
    incPath+=('pin/test/stm/',)
    incPath+=('pin/test/stm/f4/',)
    incPath+=('pin/test/stm/f4/inc/',)
    incPath+=('pin/inc/',)
    incPath+=('port/inc/',)
    linkLibs +=(drvLib, 'pthread')
    linkFlags = ''
    ccFlags ='-DSTM32F407xx -DSTM32 -DUNIT_TEST '
elif target == 'emlib':
    libFiles = ()
    ccFlags  = ' '
    print("Building all embedded libraries")
    arch = {'v6-m':None,
            'v7-m':None,
            'v7e-m':('fpv5-d16','fpv5-sp-d16',)}
    binFolder = ('lib',)+tuple(binFolder.split('/')[1:])
    binFolder = os.sep.join(binFolder)
    linkFlags = '-Xlinker -Map=output.map'
    compile_opt = xcompile_options
else:
    print("Unknown target {0}".format(target))
    sys.exit()

# Build environment
env = Environment()

# Include path for headers
env.Append(CPPPATH=[incPath])

# Ensure binaries are placed in the "binaries" directory
binaries_dir = 'binaries'
if not os.path.exists(binaries_dir):
    os.makedirs(binaries_dir)


# Build library
linkLibs += ('CppUTest','CppUTestExt')
print("Get source for libcut")
cutFiles = getSrcFromFolder(cutFolders,'*.c',binFolder)
print("Get source for testCut")
testCutFiles = getSrcFromFolder(testCutFolders,'r*_test.cpp',binFolder)
print(testCutFiles)
print("Get source for genTest")
mainFile = getSrcFromFolder(genTestFolders,'AllTests.cpp',binFolder)
print("Get source for testCom")
testComFiles  = getSrcFromFolder((googletest_framework_root,), "googletest/src/gtest-all.cc",binFolder)
testComFiles  += getSrcFromFolder((googletest_framework_root,), "googlemock/src/gmock-all.cc",binFolder)

googlelibs= getSrcFromFolder((googletest_framework_root+"/lib/",), "*.a", ".")
googleLib=Dir(googletest_framework_root+"/lib/")
linkLibs+=(googlelibs)


libcut = env.StaticLibrary('libcut', [cutFiles])
libtestCut = env.StaticLibrary('libTest_cut', [testCutFiles])
#libtestCom = env.StaticLibrary('libTestCom', [testComFiles])

# List of executables and their sources
executables = []
for f in testCutFiles:
    of=f.split(os.sep)[-1].split('.')[0]
    name = of.split("_")[0]
    print("Build executable %s" % name)
    executables.append((name, f))
print(executables)

# Build each executable and place them in the binaries folder
for exe_name, source in executables:
    env.Program(target=os.path.join(binaries_dir, exe_name), source=[source, libcut, libtestCut, googlelibs])

# Default target
Default([os.path.join(binaries_dir, exe_name) for exe_name, _ in executables])
 
