#COMAKE2 edit-mode: -*- Makefile -*-
####################64Bit Mode####################
ifeq ($(shell uname -m),x86_64)
CC=gcc
CXX=g++
CXXFLAGS=-g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp
CFLAGS=-g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -fPIC \
  -pthread
CPPFLAGS=-D_GNU_SOURCE \
  -D__STDC_LIMIT_MACROS \
  -DNDEBUG \
  -DLUNA_VERSION=\"undefined\"
INCPATH=-I./ \
  -I../tools/include \
  -I../tools/include/eigen \
  -I../tools/include/common
DEP_INCPATH=

#============ CCP vars ============
CCHECK=@ccheck.py
CCHECK_FLAGS=
PCLINT=@pclint
PCLINT_FLAGS=
CCP=@ccp.py
CCP_FLAGS=


#COMAKE UUID
COMAKE_MD5=03ff9ddefb1c1c32d70626b0e747baca  COMAKE


.PHONY:all
all:comake2_makefile_check dfgbdt 
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mall[0m']"
	@echo "make all done"

.PHONY:comake2_makefile_check
comake2_makefile_check:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mcomake2_makefile_check[0m']"
	#in case of error, update 'Makefile' by 'comake2'
	@echo "$(COMAKE_MD5)">comake2.md5
	@md5sum -c --status comake2.md5
	@rm -f comake2.md5

.PHONY:ccpclean
ccpclean:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mccpclean[0m']"
	@echo "make ccpclean done"

.PHONY:clean
clean:ccpclean
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mclean[0m']"
	rm -rf dfgbdt
	rm -rf ./output/bin/dfgbdt
	rm -rf dfgbdt_data.o
	rm -rf dfgbdt_dfgbdt_util.o
	rm -rf dfgbdt_forest.o
	rm -rf dfgbdt_hdfs_io.o
	rm -rf dfgbdt_loss.o
	rm -rf dfgbdt_node.o
	rm -rf dfgbdt_predictor.o
	rm -rf dfgbdt_preprocessor.o
	rm -rf dfgbdt_runner.o
	rm -rf dfgbdt_sampling.o
	rm -rf dfgbdt_tree.o
	rm -rf external/dfgbdt_config.o
	rm -rf external/dfgbdt_file_common.o
	rm -rf external/dfgbdt_flags.o
	rm -rf external/dfgbdt_hdfs_file.o
	rm -rf external/dfgbdt_load_hdfs_data.o
	rm -rf external/dfgbdt_sink_hdfs_data.o
	rm -rf external/dfgbdt_system.o
	rm -rf external/dfgbdt_wait_group.o

.PHONY:dist
dist:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdist[0m']"
	tar czvf output.tar.gz output
	@echo "make dist done"

.PHONY:distclean
distclean:clean
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdistclean[0m']"
	rm -f output.tar.gz
	@echo "make distclean done"

.PHONY:love
love:
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mlove[0m']"
	@echo "make love done"

dfgbdt:dfgbdt_data.o \
  dfgbdt_dfgbdt_util.o \
  dfgbdt_forest.o \
  dfgbdt_hdfs_io.o \
  dfgbdt_loss.o \
  dfgbdt_node.o \
  dfgbdt_predictor.o \
  dfgbdt_preprocessor.o \
  dfgbdt_runner.o \
  dfgbdt_sampling.o \
  dfgbdt_tree.o \
  external/dfgbdt_config.o \
  external/dfgbdt_file_common.o \
  external/dfgbdt_flags.o \
  external/dfgbdt_hdfs_file.o \
  external/dfgbdt_load_hdfs_data.o \
  external/dfgbdt_sink_hdfs_data.o \
  external/dfgbdt_system.o \
  external/dfgbdt_wait_group.o
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt[0m']"
	$(CXX) dfgbdt_data.o \
  dfgbdt_dfgbdt_util.o \
  dfgbdt_forest.o \
  dfgbdt_hdfs_io.o \
  dfgbdt_loss.o \
  dfgbdt_node.o \
  dfgbdt_predictor.o \
  dfgbdt_preprocessor.o \
  dfgbdt_runner.o \
  dfgbdt_sampling.o \
  dfgbdt_tree.o \
  external/dfgbdt_config.o \
  external/dfgbdt_file_common.o \
  external/dfgbdt_flags.o \
  external/dfgbdt_hdfs_file.o \
  external/dfgbdt_load_hdfs_data.o \
  external/dfgbdt_sink_hdfs_data.o \
  external/dfgbdt_system.o \
  external/dfgbdt_wait_group.o -Xlinker "-("   -lunwind \
  -ltcmalloc_and_profiler \
  -pthread \
  -lhiredis \
  -lrclient \
  -lrt \
  -ldl \
  -lutil \
  -lnsl \
  -lyaml-cpp \
  -fopenmp \
  -ljsoncpp \
  -lglog \
  -lgflags \
  -lboost_filesystem \
  -lboost_python \
  -lboost_system \
  -lboost_iostreams \
  -L \
  ../tools/lib \
  -lmpi_cxx \
  -lmpi \
  -lopen-rte \
  -lopen-pal \
  -Wl,--export-dynamic -Xlinker "-)" -o dfgbdt
	mkdir -p ./output/bin
	cp -f dfgbdt ./output/bin

dfgbdt_data.o:data.cpp \
  data.h \
  dfgbdt_util.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_data.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_data.o data.cpp

dfgbdt_dfgbdt_util.o:dfgbdt_util.cpp \
  dfgbdt_util.h \
  external/config.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_dfgbdt_util.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_dfgbdt_util.o dfgbdt_util.cpp

dfgbdt_forest.o:forest.cpp \
  forest.h \
  tree.h \
  node.h \
  loss.h \
  data.h \
  dfgbdt_util.h \
  preprocessor.h \
  hdfs_io.h \
  external/load_hdfs_data.h \
  external/hdfs_file.h \
  external/file_common.h \
  external/sink_hdfs_data.h \
  sampling.h \
  predictor.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_forest.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_forest.o forest.cpp

dfgbdt_hdfs_io.o:hdfs_io.cpp \
  hdfs_io.h \
  dfgbdt_util.h \
  external/load_hdfs_data.h \
  external/hdfs_file.h \
  external/file_common.h \
  external/sink_hdfs_data.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_hdfs_io.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_hdfs_io.o hdfs_io.cpp

dfgbdt_loss.o:loss.cpp \
  loss.h \
  data.h \
  dfgbdt_util.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_loss.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_loss.o loss.cpp

dfgbdt_node.o:node.cpp \
  node.h \
  loss.h \
  data.h \
  dfgbdt_util.h \
  preprocessor.h \
  hdfs_io.h \
  external/load_hdfs_data.h \
  external/hdfs_file.h \
  external/file_common.h \
  external/sink_hdfs_data.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_node.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_node.o node.cpp

dfgbdt_predictor.o:predictor.cpp \
  predictor.h \
  data.h \
  dfgbdt_util.h \
  forest.h \
  tree.h \
  node.h \
  loss.h \
  preprocessor.h \
  hdfs_io.h \
  external/load_hdfs_data.h \
  external/hdfs_file.h \
  external/file_common.h \
  external/sink_hdfs_data.h \
  sampling.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_predictor.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_predictor.o predictor.cpp

dfgbdt_preprocessor.o:preprocessor.cpp \
  preprocessor.h \
  data.h \
  dfgbdt_util.h \
  hdfs_io.h \
  external/load_hdfs_data.h \
  external/hdfs_file.h \
  external/file_common.h \
  external/sink_hdfs_data.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_preprocessor.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_preprocessor.o preprocessor.cpp

dfgbdt_runner.o:runner.cpp \
  runner.h \
  forest.h \
  tree.h \
  node.h \
  loss.h \
  data.h \
  dfgbdt_util.h \
  preprocessor.h \
  hdfs_io.h \
  external/load_hdfs_data.h \
  external/hdfs_file.h \
  external/file_common.h \
  external/sink_hdfs_data.h \
  sampling.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_runner.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_runner.o runner.cpp

dfgbdt_sampling.o:sampling.cpp \
  sampling.h \
  node.h \
  loss.h \
  data.h \
  dfgbdt_util.h \
  preprocessor.h \
  hdfs_io.h \
  external/load_hdfs_data.h \
  external/hdfs_file.h \
  external/file_common.h \
  external/sink_hdfs_data.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_sampling.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_sampling.o sampling.cpp

dfgbdt_tree.o:tree.cpp \
  tree.h \
  node.h \
  loss.h \
  data.h \
  dfgbdt_util.h \
  preprocessor.h \
  hdfs_io.h \
  external/load_hdfs_data.h \
  external/hdfs_file.h \
  external/file_common.h \
  external/sink_hdfs_data.h \
  sampling.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mdfgbdt_tree.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o dfgbdt_tree.o tree.cpp

external/dfgbdt_config.o:external/config.cpp \
  external/config.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mexternal/dfgbdt_config.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o external/dfgbdt_config.o external/config.cpp

external/dfgbdt_file_common.o:external/file_common.cpp \
  external/file_common.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mexternal/dfgbdt_file_common.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o external/dfgbdt_file_common.o external/file_common.cpp

external/dfgbdt_flags.o:external/flags.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mexternal/dfgbdt_flags.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o external/dfgbdt_flags.o external/flags.cpp

external/dfgbdt_hdfs_file.o:external/hdfs_file.cpp \
  external/hdfs_file.h \
  external/file_common.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mexternal/dfgbdt_hdfs_file.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o external/dfgbdt_hdfs_file.o external/hdfs_file.cpp

external/dfgbdt_load_hdfs_data.o:external/load_hdfs_data.cpp \
  external/load_hdfs_data.h \
  external/hdfs_file.h \
  external/file_common.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mexternal/dfgbdt_load_hdfs_data.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o external/dfgbdt_load_hdfs_data.o external/load_hdfs_data.cpp

external/dfgbdt_sink_hdfs_data.o:external/sink_hdfs_data.cpp \
  external/sink_hdfs_data.h \
  external/hdfs_file.h \
  external/file_common.h
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mexternal/dfgbdt_sink_hdfs_data.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o external/dfgbdt_sink_hdfs_data.o external/sink_hdfs_data.cpp

external/dfgbdt_system.o:external/system.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mexternal/dfgbdt_system.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o external/dfgbdt_system.o external/system.cpp

external/dfgbdt_wait_group.o:external/wait_group.cpp
	@echo "[[1;32;40mCOMAKE:BUILD[0m][Target:'[1;32;40mexternal/dfgbdt_wait_group.o[0m']"
	$(CXX) -c $(INCPATH) $(DEP_INCPATH) $(CPPFLAGS) -g \
  -O3 \
  -pipe \
  -Wextra \
  -Wall \
  -Wno-parentheses \
  -Wno-literal-suffix \
  -Wno-unused-parameter \
  -Wno-unused-local-typedefs \
  -fPIC \
  -pthread \
  -std=c++11 \
  -Wno-ignored-qualifiers \
  -fopenmp  -o external/dfgbdt_wait_group.o external/wait_group.cpp

endif #ifeq ($(shell uname -m),x86_64)


