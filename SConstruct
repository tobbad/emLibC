import glob
import os
import sys
import re

def checkDirEnding(fdir):
    return fdir if fdir[-1] == os.sep else fdir+os.sep

def getSrcFromFolder(srcDirs, srcPattern, trgtDir):
    #print("Extract files from " , srcDirs)
    res=[]
    trgtDir = checkDirEnding(trgtDir)
    for srcF in srcDirs:
        srcF = checkDirEnding(srcF)
        print("Process %s folder" % srcF)
        for f in glob.glob(srcF+srcPattern):
            res.append(trgtDir+f)
    return tuple(res)

def RegisterSrcFolderInEnv(srcDirs, env, trgtDir):
    for srcF in srcDirs:
        print("Register folder %s" % (srcF))
        srcF = checkDirEnding(srcF)
        env.VariantDir(trgtDir+srcF, srcF, duplicate=0)

def modify_chip_definitions(files=None, define_name='PERIPH_BASE'):
    keyPtr1 = re.compile(r'#if[\s]+!defined\((.*)\)')
    keyPtr2 = re.compile(r'#define[\s]+(%s)[\s]+([0-9xU]+)[\s]+(.*)' %define_name) 
    if files is None:
        files = ('pin','test','stm','f4','inc','stm32f4[0-9]*')
    for f in glob.glob(os.sep.join(files)):
        with open(f, 'rw') as fd:
            res = []
            for line in fd.readlines():
                line = line.strip()
                mtch = keyPtr1.match(line)
                if mtch is not None:
                    #print("file %s is already modified" % f)
                    res = None                    
                    break
                mtch = keyPtr2.match(line)
                if mtch is not None:
                    res.append("#if !defined(%s)" % define_name)
                res.append(line)
                if mtch is not None:
                    res.append("#endif")
        if res is not None:
            print("Rewrite %s" % f)
            with open(f, 'w') as fd:
                fd.write(os.linesep.join(res))               

modify_chip_definitions()
testComLib = 'ctest'
cutLib  =   'cut'
drvLib  =   'drv'
debug = False

xcompile_options = {
    "CC"    : "arm-none-eabi-gcc",
    "CXX"   : "arm-none-eabi-g++",
    "LD"    : "arm-none-eabi-g++",
    "AR"    : "arm-none-eabi-ar",
    "STRIP" : "arm-none-eabi-strip",
}
compile_opt={}

#
# Google test setup from :http://www.solasistim.net/posts/scons_and_google_mock/
#
googletest_framework_root = "test/googletest"

googletest_include_paths = (
    googletest_framework_root + "/googletest",
    googletest_framework_root + "/googletest/include",
    googletest_framework_root + "/googlemock",
    googletest_framework_root + "/googlemock/include"
)

gtest_all_path = googletest_framework_root + "/googletest/src/gtest-all.cc"
gmock_all_path = googletest_framework_root + "/googlemock/src/gmock-all.cc"

if ARGUMENTS.get('debug', '0') == '1':
    print("*** Debug build...")
    binFolder = 'bin/Debug/'
    debug = True
else:
    print("*** Release build...")
    binFolder = 'bin/Release/'

target = ARGUMENTS.get('target', '')
testComFiles =()
testCutFiles = ()
cutFolders =()
testCutFolders = ()
genTestFolders = ('./test/',)
incPath  = ('inc/',)
incPath  += ('mcal/',)

drvFiles =()
drvFolder =()
linkLibs =(cutLib,testComLib)

if  target == 'test_common':
    print("Create common tests.")
    cutFolders += ('./common/src/',)
    testCutFolders = ('./common/test/',)
    ccFlags  = '-DUNIT_TEST '
    incPath+=('common/inc/',)
    incPath+=('common/test/',)
    incPath  += ('test/',)
    incPath+=(googletest_include_paths,)
    linkLibs +=('pthread',)   
    linkFlags = '-Xlinker -Map=output.map'
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

#linkLibs += ('CppUTest','CppUTestExt')
testCutFiles += getSrcFromFolder(testCutFolders,'*test.cpp',binFolder)
testComFiles += getSrcFromFolder(genTestFolders,'AllTests.cpp',binFolder)
testComFiles += getSrcFromFolder((googletest_framework_root,), "googletest/src/gtest-all.cc",binFolder)
testComFiles += getSrcFromFolder((googletest_framework_root,), "googlemock/src/gmock-all.cc",binFolder)
cutFiles  = getSrcFromFolder(cutFolders,'*.cpp',binFolder)
cutFiles += getSrcFromFolder(cutFolders,'*.c',binFolder)
print(testComFiles)
print(testCutFiles)
print(cutFiles)
print(linkLibs)
libPath  = binFolder
ccDebFlags = '-g '
ccFlags  += '-Wall ' + ("" if not debug else " %s" % ccDebFlags)
cflags  =" -std=c11"
cxxflags=" -std=c++1z"
env = Environment(variant_dir=binFolder,
                  LIBPATH=binFolder,
                  LIBS=linkLibs,
                  CPPPATH=incPath, CCFLAGS=ccFlags,
                  CFLAGS=cflags, CXXFLAGS=cxxflags,
                  LINKFLAGS=linkFlags, **compile_opt)

RegisterSrcFolderInEnv(cutFolders, env, binFolder)
env.Library(target=binFolder+cutLib, source= cutFiles)

if len(drvFiles)>0:
	print("Build driver library")
	RegisterSrcFolderInEnv(drvFolder, env, binFolder)
	env.Library(target=binFolder+drvLib, source= drvFiles)

RegisterSrcFolderInEnv(testCutFolders, env, binFolder)

RegisterSrcFolderInEnv(genTestFolders, env, binFolder)
env.Library(target=binFolder+testComLib, source= testComFiles)

if target != 'emlib':
    for f in testCutFiles:
        of=f.split(os.sep)[-1].split('.')[0]
        print("Build executable %s" % of)
        env.Program(binFolder+of, (f,))
else:
    env.Program(binFolder+of, (f,))
