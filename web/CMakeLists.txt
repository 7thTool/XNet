#web/CMakeLists.txt
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

CMAKE_POLICY(SET CMP0015 NEW)

MESSAGE(STATUS "web v${XSERVER_VERSION_MAJOR}.${XSERVER_VERSION_MINOR}.${XSERVER_VERSION_BUILD}")

#CMAKE_C_COMPILER：指定C编译器
#CMAKE_CXX_COMPILER：指定C++编译器
#原型1,2,3,4,5,6
ADD_DEFINITIONS(-DXSERVER_PROTOTYPE=1)
IF(WIN32)
	#需要兼容XP时,定义_WIN32_WINNT 0x0501
	ADD_DEFINITIONS(-D_WIN32_WINNT=0x0601)
	#不用加EXPORTS定义，cmake会自动加XServer_EXPORTS
  	#ADD_DEFINITIONS(-DXSERVER_EXPORTS)
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT")
	SET(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT")
	#VS版本的控制可以采用如下变量
	IF(MSVC_VERSION GREATER 1500)
		ADD_COMPILE_OPTIONS(/bigobj)
	ENDIF()
ELSE()
	ADD_DEFINITIONS(-std=c++11)
	ADD_DEFINITIONS(-fpermissive)
	ADD_DEFINITIONS(-fPIC)
	#-w的意思是关闭编译时的警告，也就是编译后不显示任何warning
	#-Wall选项意思是编译后显示所有警告
	#-W选项类似-Wall，会显示警告，但是只显示编译器认为会出现错误的警告
	#调试信息格式主要有下面几种：stabs，COFF，PE-COFF，OMF，IEEE-695和DWARF
	#其中DWARF在Linux中被普遍使用，dwarf2对dwarf1的改变很大，dwarf3大多是对dwarf2的扩充，可以支持C、C++、JAVA、Fortran等语言
	#使用readelf –w* XServer命令，*是调试节名的第一个字母，如-wi就是查看.debug_info节的内容，-wl就是查看.debug_line节的内容
	#-g、-ggdb、-g3和-ggdb3，-g产生OS native format的debug信息，GDB可以使用之。而-ggdb产生的debug信息更倾向于给GDB使用的
	#如果你用的GDB调试器，那么使用-ggdb选项。如果是其他调试器，则使用-g。3只是debug信息级别，3这个级别可以调试宏。
	#SET(CMAKE_CXX_FLAGS_DEBUG "-std=c++11 ${CMAKE_CXX_FLAGS_DEBUG} -O0 -w -gdwarf-2 -ggdb3")
	#SET(CMAKE_CXX_FLAGS_RELEASE "-std=c++11 ${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall -DNODEBUG -gdwarf-2 -ggdb")
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -W -g3")
	SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -W")
	SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -W -gdwarf-3 -g")
	SET(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -W")
  	INCLUDE_DIRECTORIES(/usr/local/include)
	LINK_DIRECTORIES(/usr/local/lib)
ENDIF()

IF(WIN32)
ELSE()
#以上写法都没用，要LINUX要完全使用BOOST静态库，需要编译Boost时指定-fPIC
ENDIF()
#设置需要的boost 组件
#SET(BOOST_COMPONENTS log_setup log regex date_time chrono system filesystem thread)
#使用cmake find_package 查找boost库位置
#FIND_PACKAGE(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
FIND_PACKAGE(Boost REQUIRED)
IF(Boost_FOUND)
	MESSAGE(STATUS "Boost library status:")
	MESSAGE(STATUS "     version: ${Boost_VERSION}")
	MESSAGE(STATUS "     include path: ${Boost_INCLUDE_DIRS}")
	MESSAGE(STATUS "     library path: ${Boost_LIBRARY_DIRS}")
	IF(WIN32)
	ELSE()
  	ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)
 	ENDIF()
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
	LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
ELSE()
	MESSAGE(FATAL_ERROR "BOOST library not found")
ENDIF()

SET(web_SRCS
	./main.cpp 
) 

#添加头文件搜索路径
INCLUDE_DIRECTORIES(./ ../ ../..)
#添加库文件搜索路径
LINK_DIRECTORIES(./ ../lib)

IF(WIN32)
	#
ELSE()
	FIND_LIBRARY(BOOST_LOG_SETUP boost_log_setup ${Boost_LIBRARY_DIRS})
	FIND_LIBRARY(BOOST_LOG boost_log ${Boost_LIBRARY_DIRS})
	FIND_LIBRARY(BOOST_REGEX boost_regex ${Boost_LIBRARY_DIRS})
	FIND_LIBRARY(BOOST_DATE_TIME boost_date_time ${Boost_LIBRARY_DIRS})
	FIND_LIBRARY(BOOST_CHRONO boost_chrono ${Boost_LIBRARY_DIRS})
	# 链接动态库
	#SET (EXTRA_LIBS ${EXTRA_LIBS} pthread)
	# 链接静态库
	SET (EXTRA_LIBS ${EXTRA_LIBS} ${BOOST_LOG_SETUP} ${BOOST_LOG} ${BOOST_REGEX} ${BOOST_DATE_TIME} ${BOOST_CHRONO}
	 boost_system boost_filesystem boost_thread pthread)
ENDIF()

SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#检查一下web的链接情况shell命令：ldd src/web
ADD_EXECUTABLE(web ${web_SRCS})
#这个指令可以用来为target 添加需要链接的共享库
TARGET_LINK_LIBRARIES(web ${EXTRA_LIBS})

