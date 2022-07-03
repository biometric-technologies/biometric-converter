# Biometric Converter

About
-----
Biometric Converter is software that converts fingerprint images of different formats (WSQ, JPEG,
PNG, [ANSI 381](https://webstore.ansi.org/Standards/INCITS/ANSIINCITS3812004))
to fingerminutia formats ([ANSI/INCITS 378](https://tsapps.nist.gov/publication/get_pdf.cfm?pub_id=150619)
and [ISO/IEC 29794-4](http://www.iso.org/iso/catalogue_detail.htm?csnumber=62791)).
This repository serves as a formally recognized reference implementation of the
international standard.


Dependencies
------------

Building Biometric Converter requires the following dependencies:

* [NIST Biometric Image Software](https://github.com/biometric-technologies/nist-biometric-image-software-nbis) [Download](https://www.nist.gov/services-resources/software/nist-biometric-image-software-nbis) ([MIT License](https://opensource.org/licenses/MIT))
    * Requires other non-bundled dependencies, please see
      the [README](https://github.com/biometric-technologies/nist-biometric-image-software-nbis/blob/master/INSTALL_LINUX_MACOSX.txt)
      .

After build copy libraries from `{BUIDL_DIR}/lib` to `thirdparty/nbis/lib` directory.


Build And Install Library + Command-line Interface
---------------------------------------------

```bash
mkdir build
cd build
cmake ..
make install 
```

Command-line Interface Usage
---------------------------------------------

```bash
./convert -i <input_file> -o <output_file> -t <output_type>
```

| Param       | Description                                       |
|-------------|---------------------------------------------------|
| input_file  | path to image file you want to convert from       |
| output_file | path to file you want to convert to               |
| output_type | output file type format: (ANSI, ISO, ISOC, ISOCC) |

### Demo

[![Demo CLI Tool](https://j.gifs.com/jYkBkz.gif)](https://www.youtube.com/watch?v=Ke1P1PfEm30)


Library Usage
---------------------------------------------

Add `converter.h` to your target include directory and link all libraries.

Then include `converter.h` in your code and use `convert` function.

```C
#include <converter.h>
...
unsigned char* input_data, output_data;
int input_length, output_length;
char* output_type;

convert(input_data, input_length, &output_data, &output_length);
...
```

| Param         | Description                                       |
|---------------|---------------------------------------------------|
| input_data    | image data want to convert from                   |
| input_length  | image data length you want to convert from        |
| output_type   | output file type format: (ANSI, ISO, ISOC, ISOCC) |
| output_data   | output data                                       |
| output_length | output data length                                |

Build And Run Docker Image
--------------------

This command will build local docker image `biometric-converter`

```bash
cd web-service
./gradlew jibDockerBuild
```

To run docker container:

```bash
docker run -p 8080:8080 biometric-converter
```

Run Docker Image from docker.io
--------------------

You can also use existing docker image:

```bash
docker run -p 8080:8080 biometrictechnologies/biometric-converter
```

Web Service REST API Documentation
--------------------

### Convert image format to fingerminuta

#### Request

`POST /convert`

    curl --location --request POST 'http://localhost:8080/convert' \
    --header 'Content-Type: application/json' \
    --data-raw '{
    "input": "...",
    "outputType": "ANSI"
    }'

| Param         | Description                                            |
|---------------|--------------------------------------------------------|
| input         | image data in BASE64 encoding you want to convert from |
| outputType    | output file type format: (ANSI, ISO, ISOC, ISOCC)      |

#### Response

    HTTP/1.1 200 OK
    Date: Thu, 24 Feb 2022 04:00:00 GMT
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

Communication
-------------
If you found a bug and can provide steps to reliably reproduce it, or if you
have a feature request, please
[open an issue](https://github.com/biometric-technologies/biometric-converter/issues). Other
questions may be addressed to the
[Biometric Technologies project maintainers](mailto:info@iriscan.net).

License
-------
Biometric Converter is released in the GNU PUB3 Licanse. See the
[LICENSE](https://github.com/biometric-technologies/biometric-converte/blob/master/LICENSE.md)
for details.
