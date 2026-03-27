# Lazarus C++17 Transpiler — Federal Validation Container
# Torsova LLC — lazarus-systems.com
#
# Compiles and validates all 1,607 transpiled C++17 programs
# against the GnuCOBOL 3.2 test suite originals.
#
# Build context must be the lazarus-cobol-showcase/ root:
#   docker build -t lazarus-cpp17-validator -f federal-validator/Dockerfile .
#   docker run --rm lazarus-cpp17-validator
#
# Or use docker-compose from federal-validator/:
#   docker-compose up

FROM ubuntu:24.04

LABEL maintainer="Torsova LLC <info@lazarus-systems.com>"
LABEL description="Lazarus C++17 Transpiler Validation Suite"

ENV DEBIAN_FRONTEND=noninteractive

# Install build toolchain + GnuCOBOL
RUN apt-get update && apt-get install -y --no-install-recommends \
    g++ \
    make \
    gnucobol \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Verify toolchain
RUN echo "=== Toolchain ===" \
    && g++ --version | head -1 \
    && cobc --version | head -1

WORKDIR /validator

# Copy validation infrastructure
COPY federal-validator/run.sh federal-validator/test_runner.sh federal-validator/parity_harness.sh federal-validator/oracle_harness.sh federal-validator/oracle_autofixer.py federal-validator/Makefile federal-validator/autofixer.py ./
COPY federal-validator/testsuite_oracle.json ./
COPY federal-validator/runtime/ runtime/

# Copy transpiler output: 1,607 clean + 1,607 hardened C++17 files
COPY output/ cpp_output/

# Copy original COBOL source: 1,607 .cbl files
COPY cobol_source/ cobol_source/

RUN ln -s cpp_output output

# Fix Windows CRLF line endings for Linux
RUN sed -i 's/\r$//' run.sh test_runner.sh parity_harness.sh oracle_harness.sh autofixer.py

RUN chmod +x run.sh test_runner.sh parity_harness.sh oracle_harness.sh

ENTRYPOINT ["./run.sh"]
