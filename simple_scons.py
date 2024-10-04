import glob
import os
import sys
import re


def normalizeLieEnding(fdir):
    # Add trailing / to path
    print("Tye od dir is %s" % type(fdir))
    return fdir if fdir[-1] == os.sep else fdir+os.sep


def getSrcFromFolder(srcDirs, srcPattern, trgtDir):
    #print("Extract files from " , srcDirs)
    res=[]
    trgtDir = normalizeLieEnding(trgtDir)
    print(trgtDir)
    for srcF in srcDirs:
        srcF = normalizeLieEnding(srcF)
        print("Process %s folder" % srcF)
        for f in glob.glob(srcF+srcPattern):
            res.append(trgtDir+f)
    return tuple(res)

def RegisterSrcFolderInEnv(srcDirs, env, trgtDir):
    for srcF in srcDirs:
        print("Register folder %s" % (srcF))
        srcF = normalizeLieEnding(srcF)
        env.VariantDir(trgtDir+srcF, srcF, duplicate=0)


def RegisterSrcFolderInEnv(srcDirs, env, trgtDir):
    for srcF in srcDirs:
        print("Register folder %s" % (srcF))
        srcF = normalizeLieEnding(srcF)
        env.VariantDir(trgtDir+srcF, srcF, duplicate=0)

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

compile_opt={}
binFolder = bin
testComLib = 'ctest'
cutLib  =   'cut'
linkLibs =(testComLib, cutLib)
incPath  = ('inc/',)
cutFolders =()
testCutFiles = ()


debug = ARGUMENTS.get('debug', '0')
target = ARGUMENTS.get('target', '0')
print("Build target is %s" % (target))
print("Debug is %s" % (debug))

if  debug == '1':
    print("*** Debug build...")
    debug = True
else:
    print("*** Release build...")


if  target == 'simple':
    print("Create hello world tests.")
    cutFolders += ('./common/src/',)
    testCutFolders = ('./common/test/',)
    ccFlags  = '-DUNIT_TEST '
    linkFlags = ''

    incPath+=('common/inc/',)
    incPath+=('common/test/',)
    incPath  += ('test/',)
    incPath+=(googletest_include_paths,)
    linkLibs +=('pthread',)   
    linkFlags = '-Xlinker -Map=output.map'
    debug = True
else:
    print("Unknown target {0}".format(target))
    sys.exit()

testCutFiles += getSrcFromFolder(testCutFolders,'simple_test.cpp',binFolder)
testComFiles += getSrcFromFolder(testCutFolders,'common/simple*.c*',binFolder)
testComFiles += getSrcFromFolder(genTestFolders,'AllTests.cpp',binFolder)
testComFiles += getSrcFromFolder((googletest_framework_root,), "googletest/src/gtest-all.cc",binFolder)
testComFiles += getSrcFromFolder((googletest_framework_root,), "googlemock/src/gmock-all.cc",binFolder)

ccDebFlags = '-g '
ccFlags  += '-Wall ' + ("" if not debug else " %s" % ccDebFlags)
cflags  =" -std=c11 -fstack-protector-strong"
cxxflags=" -std=c++1z"

env = Environment(variant_dir=binFolder,
                  LIBPATH=binFolder,
                  LIBS=linkLibs,
                  CPPPATH=incPath, CCFLAGS=ccFlags,
                  CFLAGS=cflags, CXXFLAGS=cxxflags,
                  LINKFLAGS=linkFlags, **compile_opt)

cutFiles = getSrcFromFolder(cutFolders,'simple_test.c',binFolder)
    
RegisterSrcFolderInEnv(cutFolders, env, binFolder)
RegisterSrcFolderInEnv(testCutFolders, env, binFolder)

Program('hello.c')
