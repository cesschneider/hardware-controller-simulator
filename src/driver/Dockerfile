FROM ubuntu:22.04

# Install build tools and libcurl
RUN apt-get update && apt-get install -y g++ make libcurl4-openssl-dev

WORKDIR /usr/src/app

# Copy source files
COPY main.cpp .
COPY HardwareAPI.h .
COPY HardwareAPI.cpp .
COPY CommandValidator.h .
COPY CommandValidator.cpp .

# Compile
RUN g++ main.cpp HardwareAPI.cpp CommandValidator.cpp -o driver -lcurl

EXPOSE 6001

CMD ["./driver"]
