import glob
import os
import sys
import re

cwd= os.getcwd()
print("Start in %s" % cwd)
remove_cut_files = ("_time.c",)



env = Environment()

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
            fname=f.split("/")[-1]
            if fname in remove_cut_files:
                print("  Skip %s" %fname)
                continue
            print("  Append %s" %(trgtDir+f)) 
            res.append(trgtDir+f)
    return tuple(res)

def RegisterSrcFolderInEnv(srcDirs, env, trgtDir):
    for srcF in srcDirs:
        print("Register folder %s" % (trgtDir+srcF))
        srcF = normalizeLineEnding(srcF)
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

#modify_chip_definitions()
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
googletest_framework_root = "./test/googletest"

googletest_include_paths = (
    googletest_framework_root + "/googletest/include",
    googletest_framework_root + "/googlemock/include")

gtest_all_path = googletest_framework_root + "/googletest/src/gtest-all.cc"
gmock_all_path = googletest_framework_root + "/googlemock/src/gmock-all.cc"

debug = ARGUMENTS.get('debug', '0')
target = ARGUMENTS.get('target', '0')
print("Build target is %s" % (target))
print("Debug is %s" % (debug))

testComFiles =()
testCutFiles = ()
cutFolders =()
testCutFolders = ()
genTestFolders = ('./test/',)
incPath  = ()

linkLibs =()

if  target == 'test_common':
    print("Create common tests.")
    cutFolders += ('./common/src/',)
    testCutFolders = ('./common/test/',)
    ccFlags  = '-DUNIT_TEST '
    incPath += ('./common/inc/',)
    incPath += ('./common/test/',)
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

if debug:
    print("*** Debug build...")
    binFolder = './'
    print("Include path %s" % str(incPath))
else:
    print("*** Release build...")
    binFolder = './'


linkLibs += ('CppUTest','CppUTestExt')
print("Get source for libcut")
cutFiles = getSrcFromFolder(cutFolders,'*.c',binFolder)
print("Get source for testCut")
testCutFiles = getSrcFromFolder(testCutFolders,'*_test.cpp',binFolder)
print("Get source for genTest")
mainFile = getSrcFromFolder(genTestFolders,'AllTests.cpp',binFolder)
print("Get source for testCom")
testComFiles  = getSrcFromFolder((googletest_framework_root,), "googletest/src/gtest-all.cc",binFolder)
testComFiles  += getSrcFromFolder((googletest_framework_root,), "googlemock/src/gmock-all.cc",binFolder)

googlelibs= getSrcFromFolder((googletest_framework_root+"/lib/",), "*.a", ".")
googleLib=Dir(googletest_framework_root+"/lib/")
linkLibs+=(googlelibs)
lib  =   'cut'
env.StaticLibrary(target = lib, source = cutFiles)
linkLibs += ("lib"+lib,)

lib  =   'testcom'
env.StaticLibrary(target = lib, source = testComFiles)
linkLibs += ("lib"+lib,)

print("LinkLibs =%s" % str(linkLibs))

print("Include path : %s" % " ".join(i for i in incPath)) 
print("cutFiles     : %s" % " ".join(i for i in cutFiles))
print("testCutFiles : %s" % " ".join(i for i in testCutFiles))
print("testComFiles : %s" % " ".join(i for i in testComFiles))
print("libGoogle    : %s" % " ".join(i for i in googlelibs))
print("linkLibs     : %s" % " ".join(i for i in linkLibs))
#env.Environment(CPPPATH=incPath)
#env.Environment(CPPPATH=incPath)
print(str(incPath))
env["CPPPATH"] =  str(incPath) 
libPath  = binFolder
ccDebFlags = '-g '
ccFlags  += '-Wall ' + ("" if not debug else " %s" % ccDebFlags)
cflags  =" -std=c11 -fstack-protector-strong"
cxxflags=" -std=c++1z"
print('type(env["CPPPATH"])=%s'% type(env["CPPPATH"]))
print('env["CPPPATH"]=%s'% env["CPPPATH"])
print('type(env[CPPPATH])=%s'% type(env["CPPPATH"]))
print('env[CPPPATH]=%s'% env["CPPPATH"])
#env["CPPPATH"] = +="."
print("******DUMP ENV")
print(type(env))
env.Dump()

if target != 'emlib':
    print(testCutFiles)
    for f in testCutFiles:
        of=f.split(os.sep)[-1].split('.')[0]
        name = of.split("_")[0]
        print("Build executable %s" % name)
        env.Program(
            VariantDir=("build/"+name,),
            src=[f, mainFile],
            target = name,
            libs= linkLibs )
 
env.Dump()
