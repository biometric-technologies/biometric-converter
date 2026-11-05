package net.iriscan.bcws.controller

import com.sun.jna.ptr.IntByReference
import com.sun.jna.ptr.PointerByReference
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.coroutineScope
import net.iriscan.bcws.dto.*
import net.iriscan.bcws.extension.decodeBase64
import net.iriscan.bcws.extension.encodeBase64
import net.iriscan.bcws.lib.ConverterFactory
import net.iriscan.bcws.lib.FileFormat
import org.springframework.http.HttpStatus
import org.springframework.web.bind.annotation.CrossOrigin
import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RequestBody
import org.springframework.web.bind.annotation.RestController
import org.springframework.web.server.ResponseStatusException

/**
 * @author Slava Gornostal
 */
@CrossOrigin
@RestController
class ConvertController {

    private val converter = ConverterFactory.instance

    @PostMapping("/convert")
    fun convert(@RequestBody request: Request): Response =
        Response(convertOne(request.input, request.inputType, request.outputType, request.imageResX, request.imageResY))

    @PostMapping("/convert-batch")
    suspend fun convertBatch(@RequestBody request: BatchRequestList): BatchResponseList = coroutineScope {
        val converted = request.data
            .map {
                async(Dispatchers.Default) {
                    BatchResponse(
                        it.id,
                        convertOne(it.input, it.inputType, it.outputType, it.imageResX, it.imageResY)
                    )
                }
            }
            .awaitAll()
        BatchResponseList(converted)
    }

    private fun convertOne(
        inputBase64: String,
        inputType: FileFormat,
        outputType: FileFormat,
        imageResX: Int = 0,
        imageResY: Int = 0
    ): String {
        val input = inputBase64.decodeBase64()
        val out = PointerByReference()
        val outLength = IntByReference()
        // The library keeps the error message per thread, so read it on the
        // thread that made the call
        val rc = when {
            inputType.isImage() && outputType.isMinutae() ->
                converter.img2fmr(input, input.size, outputType.libraryName(), out, outLength)

            // The resolution matters for ISO card input, which does not carry it;
            // 0 means 500 ppi
            inputType.isMinutae() && outputType.isMinutae() ->
                converter.fmr2fmr_iso_card(
                    input, input.size, out, outLength,
                    inputType.libraryName(), outputType.libraryName(), imageResX, imageResY
                )

            else -> throw ResponseStatusException(
                HttpStatus.BAD_REQUEST,
                "Conversion from $inputType to $outputType is not supported."
            )
        }
        if (rc != 0) {
            throw ResponseStatusException(HttpStatus.BAD_REQUEST, converter.converter_last_error())
        }

        val data = out.value
        try {
            return data.getByteArray(0, outLength.value).encodeBase64()
        } finally {
            converter.converter_free(data)
        }
    }

}