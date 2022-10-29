# Biometric Converter

About
-----
Biometric converter is software that converts fingerprint images (WSQ, JPEG, JPEG2000,
PNG) to finger minutiae formats ([ANSI/INCITS 378](https://tsapps.nist.gov/publication/get_pdf.cfm?pub_id=150619)
and [ISO/IEC 29794-4](http://www.iso.org/iso/catalogue_detail.htm?csnumber=62791)) and finger minutiae format between
each other.
This repository serves as a formally recognized reference implementation of the international standard.

Build / Installation
------------

### Prerequisites

* [Git](https://git-scm.com/downloads)
* [Cmake 3.10.2+](https://cmake.org/install/)
* [GCC 8+](https://gcc.gnu.org/install/)
* [JDK 11*](https://www.oracle.com/java/technologies/javase/jdk11-archive-downloads.html) optional, required for WS
* [Docker*](https://docs.docker.com/engine/install/) optional

### Install dependencies

#### NBIS

1. Clone [repository](https://github.com/biometric-technologies/nist-biometric-image-software-nbis.git)   
   `git clone https://github.com/biometric-technologies/nist-biometric-image-software-nbis`
2. Follow commands to install:

```bash
./setup.sh <INSTALLATION DIR> --without-X11 [--32 | --64]  
make config
make it
make install LIBNBIS=no
```  

#### BIOMDI

1. Clone [repository](https://github.com/usnistgov/biomdi.git)   
   `git clone https://github.com/usnistgov/biomdi.git`
2. Edit `common/common.mk` if you want to install to custom location
3. Follow commands to install:

```bash
make  
make install
```

### Build and install convert library with CLI

1. Modify `CMakeLists.txt`, set path to dependencies libraries and installation directory

2. Run commands to install:

```bash
mkdir build
cd build
cmake ..
make install 
```

### Build Web Service

1. Run commands to install:

```bash
cd web-service
./gradlew clean build
```

### Build Example

1. Modify `example/CMakeLists.txt`, set path to dependencies libraries and installation directory
2. Run commands to install:

```bash
mkdir build
cd build
cmake ..
make
```

CLI Usage
---------------------------------------------

Convert file.

```bash
convert -i <input_file> -ti <input_type> -o <output_file> -to <output_type>
```

| Param       | Description                                |
|-------------|--------------------------------------------|
| input_file  | path to file you want convert from         |
| input_type  | input file type format (image or minutiae) |
| output_file | path to file you want convert to           |
| output_type | output file type format (minutiae)         |

You can also use docker image from [Docker Hub](https://hub.docker.com/r/biometrictechnologies/biometric-converter-cli)
to use CLI without building/installing software.

1. Pull image

```shell
docker pull biometrictechnologies/biometric-converter-cli
```

2. Use CLI

```shell
docker run -i --rm -v ./work:/opt/work biometrictechnologies/biometric-converter-cli convert -i <input_file> -ti <input_type> -o <output_file> -to <output_type>
```

### Demo

[![Demo CLI Tool](https://j.gifs.com/jYkBkz.gif)](https://www.youtube.com/watch?v=Ke1P1PfEm30)


Library Usage
---------------------------------------------

### Setup

1. Add library to the project, include directory with a library header and link library files.

```cmake
include_directories(/usr/local/lib/include)
link_directories(/usr/local/lib)

find_library(converter REQUIRED)

target_link_libraries(<your target> PUBLIC converter)
```

2. Import header `#include <converter.h>`
3. Use library methods

### Methods overview

#### Convert image to fingerprint minutiae format

```C
img2fmr(unsigned char *, int , char *, unsigned char **, int *)
```

| Param         | Description                                |
|---------------|--------------------------------------------|
| input_data    | image data want to convert from            |
| input_length  | image data length you want to convert from |
| output_type   | output file type format (minutiae)         |
| output_data   | output data                                |
| output_length | output data length                         |

#### Convert fingerprint minutiae to fingerprint minutiae

```C
fmr2fmr(unsigned char *, int , unsigned char **, int *, char *, char *)
```

| Param         | Description                               |
|---------------|-------------------------------------------|
| input_data    | file data want to convert from            |
| input_length  | file data length you want to convert from |
| output_data   | output data                               |
| output_length | output data length                        |
| input_type    | input file type format (minutiae)         |
| output_type   | output file type format (minutiae)        |

#### Convert fingerprint minutiae to ISO(Card/Compact Card)

```C
int fmr2fmr_iso_card(unsigned char *, int , unsigned char **, int *,char *, char *, int , int )
```

| Param         | Description                                         |
|---------------|-----------------------------------------------------|
| input_data    | file data want to convert from                      |
| input_length  | file data length you want to convert from           |
| output_data   | output data                                         |
| output_length | output data length                                  |
| input_type    | input file type format (minutiae)                   |
| output_type   | output file type format (minutiae, ISOC/ISOCC only) |
| x resolution  | image resolution x                                  |
| y resolution  | image resolution y                                  |

Web Service REST API Documentation
--------------------
Web service accepts and respond in JSON format, files should be transferred in base64 encoding.

### Convert single file

#### Request

`POST /convert`

    curl --location --request POST 'http://localhost:8080/convert' \
    --header 'Content-Type: application/json' \
    --data-raw '{
    "input": "...",
    "inputType": "WSQ"
    "outputType": "ANSI"
    }'

| Param      | Description                                            |
|------------|--------------------------------------------------------|
| input      | image data in BASE64 encoding you want to convert from |
| inputType  | input file type format (image or minutiae)             |
| outputType | output file type format (minutiae)                     |
| imageResX  | image x resolution for ISO Card format (optional)      |
| imageResY  | image y resolution for ISO Card format (optional)      |

#### Response

    HTTP/1.1 200 OK
    Date: Thu, 24 Feb 2022 04:00:00 GMT+2
    Status: 200 OK
    Connection: close
    Content-Type: application/json
    Content-Length: 2

    {
        "output": "..."
    }

| Param  | Description                |
|--------|----------------------------|
| output | BASE64 encoded output data |

### Convert batch

Convert multiple files, response array is not in the same order as requested. Use `id` field to map your files.

#### Request

`POST /convert-batch`

    curl --location --request POST 'http://localhost:8080/convert' \
    --header 'Content-Type: application/json' \
    --data-raw '{
    "data": [...]
    }'

| Param | Description                |
|-------|----------------------------|
| data  | Array of files to convert  |

File to convert

| Param      | Description                                            |
|------------|--------------------------------------------------------|
| id         | id of the requested image                              |
| input      | image data in BASE64 encoding you want to convert from |
| inputType  | input file type format (image or minutiae)             |
| outputType | output file type format (minutiae)                     |
| imageResX  | image x resolution for ISO Card format (optional)      |
| imageResY  | image y resolution for ISO Card format (optional)      |

#### Response

    HTTP/1.1 200 OK
    Date: Thu, 24 Feb 2022 04:00:00 GMT+2
    Status: 200 OK
    Connection: close
    Content-Type: application/json
    Content-Length: 2

    {
        "data": [...]
    }

| Param | Description              |
|-------|--------------------------|
| data  | Array of converted files |

Converted file

| Param  | Description                 |
|--------|-----------------------------|
| id     | id of the requested image   |
| output | BASE64 encoded output data  |

1. Pull image

```shell
docker pull biometrictechnologies/biometric-converter
```

2. Start Web Service

```shell
docker run -it -p 8080:8080 biometrictechnologies/biometric-converter
```

Communication
-------------
If you found a bug and can provide steps to reliably reproduce it, or if you
have a feature request, please
[open an issue](https://github.com/biometric-technologies/biometric-converter/issues). Other
questions may be addressed to the
[Biometric Technologies project maintainers](mailto:info@iriscan.net).

License
-------
Biometric Converter is released under the GNU PUB3 License. See the
[LICENSE](https://github.com/biometric-technologies/biometric-converte/blob/master/LICENSE.md)
for details.
