#! /bin/bash
if [[ -z $CLANG_TIDY ]]; then
	CLANG_TIDY="clang-tidy"
fi

echo "Using "$CLANG_TIDY

GLOBIGNORE='**/mbedtls/**' # do not look into mbedTLS code

echo "Ignoring files in "$GLOBIGNORE

INCLUDES="-Ibuild -Isrc -I/usr/local/opt/openssl/include"

DEFINES="-DCHECK_TEMPLATE_INSTANTIATION  -DWITH_OPENSSL"

CFLAGS="-march=native -O2 -fPIC -Wall -Wcast-qual -Wdisabled-optimization -Wformat=2 -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wstrict-overflow=5 -Wdeprecated -Wno-unused-function"

CCFLAGS="-std=c99"

CXXFLAGS="-std=c++14 -Weffc++ -Wnon-virtual-dtor -Woverloaded-virtual -Wsign-promo"

LINE_FILTER="''"

eval "$CLANG_TIDY src/**/*.{h,c} -line-filter=$LINE_FILTER -- $CFLAGS $CCFLAGS $INCLUDES"

eval "$CLANG_TIDY src/*.{hpp,cpp} src/**/*.{hpp,cpp} -line-filter=$LINE_FILTER -- $CFLAGS $CXXFLAGS $INCLUDES"

