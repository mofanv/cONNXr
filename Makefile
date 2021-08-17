
VARIABLE+=TRACE_LEVEL
HELP_TRACE_LEVEL=trace verbosity
# TRACE_LEVEL?=0

VARIABLE+=BUILDDIR
HELP_BUILDDIR=build directory
BUILDDIR?=build

VARIABLE+=PROFILINGDIR
HELP_PROFILINGDIR=profiling directory
PROFILINGDIR?=profiling

VARIABLE+=FORMAT
HELP_FORMAT=which files to format (git wildcards)
ifndef FORMAT
FORMAT+=**/*.h
FORMAT+=**/*.c
FORMAT+=!**/protobuf/**/*
FORMAT+=!**/third_party/**/*
endif

VARIABLE+=ONNX_CUSTOM
HELP_ONNX_CUSTOM=use custom onnx installation
ONNX_CUSTOM=third_party/onnx/onnx.build

VARIABLE+=ONNX_INCLUDE
HELP_ONNX_INCLUDE=which schemas to include
ifndef ONNX_INCLUDE
ONNX_INCLUDE+="^Add$$"
ONNX_INCLUDE+="^ArgMax$$"
ONNX_INCLUDE+="^BatchNormalization$$"
ONNX_INCLUDE+="^Clip$$"
ONNX_INCLUDE+="^Constant$$"
ONNX_INCLUDE+="^Conv$$"
ONNX_INCLUDE+="^GlobalAveragePool$$"
ONNX_INCLUDE+="^LeakyRelu$$"
ONNX_INCLUDE+="^MatMul$$"
ONNX_INCLUDE+="^MaxPool$$"
ONNX_INCLUDE+="^Mul$$"
ONNX_INCLUDE+="^Relu$$"
ONNX_INCLUDE+="^Reshape$$"
ONNX_INCLUDE+="^Sigmoid$$"
ONNX_INCLUDE+="^Softmax$$"
ONNX_INCLUDE+="^Transpose$$"
ONNX_INCLUDE+="^Elu$$"
ONNX_INCLUDE+="^Identity$$"
endif

VARIABLE+=ONNX_VERSION
HELP_ONNX_VERSION=which onnx version to use
ONNX_VERSION=latest

VARIABLE+=ONNX_DOMAINS
HELP_ONNX_DOMAINS=which onnx domains to use
ONNX_DOMAINS=ai.onnx

VARIABLE+=ONNX_EXCLUDE
HELP_ONNX_EXCLUDE=which schemas to exclude
ONNX_EXCLUDE=

CC=../veracruz-examples/wasi-sdk-12.0/bin/clang --sysroot=../veracruz-examples/wasi-sdk-12.0/share/wasi-sysroot
CFLAGS+=-std=c99
CFLAGS+=-Wall
CFLAGS+=-g3 -gdwarf -O2
# CFLAGS+=-Werror # CI jobs run with flag enabled
ifdef TRACE_LEVEL
CPPFLAGS+=-D "TRACE_LEVEL=$(TRACE_LEVEL)"
endif

LDFLAGS+=-g
LDLIBS+=-lm

INCDIR+=include
INCDIR+=protobuf
CPPFLAGS+=$(foreach DIR, $(INCDIR),-I $(DIR) )

SRCDIR+=src/operators
SRCDIR+=protobuf
SRCS+=$(foreach DIR, $(SRCDIR), $(shell find $(DIR) -type f -name '*.c'))
SRCS+=src/inference.c
SRCS+=src/trace.c
SRCS+=src/utils.c
SRCS+=src/test/test_utils.c
OBJS=$(SRCS:%.c=$(BUILDDIR)/%.wasm)

$(BUILDDIR)/%.wasm:%.c
	@mkdir -p $(dir $@)
	$(CC) -o $@ -c $(CFLAGS) $(CPPFLAGS) $^

$(BINARY): $(OBJS)

DEFAULT=help

.phony:connxr
HELP_connxr=build connxr binary
TARGET+=connxr
ALL+=connxr
connxr: $(BUILDDIR)/connxr.wasm
$(BUILDDIR)/connxr.wasm: $(OBJS)
	$(CC) -o $@ src/connxr.c $^ $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(LDLIBS)


.phony: clean_build
CLEAN+=clean_build
clean_build:
	rm -rf $(BUILDDIR)


include .Makefile.template
