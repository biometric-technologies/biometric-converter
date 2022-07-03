package net.iriscan.bcws.controller

import com.sun.jna.ptr.IntByReference
import com.sun.jna.ptr.PointerByReference
import net.iriscan.bcws.dto.Request
import net.iriscan.bcws.dto.Response
import net.iriscan.bcws.extension.decodeBase64
import net.iriscan.bcws.extension.encodeBase64
import net.iriscan.bcws.lib.Converter
import org.springframework.web.bind.annotation.CrossOrigin
import org.springframework.web.bind.annotation.PostMapping
import org.springframework.web.bind.annotation.RequestBody
import org.springframework.web.bind.annotation.RestController

/**
 * @author Slava Gornostal
 */
@CrossOrigin
@RestController
class ConvertController {

    private val converter = Converter.instance

    @PostMapping("/convert")
    fun convert(@RequestBody request: Request): Response {
        val input = request.input.decodeBase64()
        val out = PointerByReference()
        val outLength = IntByReference()
        converter.convert(input, input.size, request.outputType.name, out, outLength)
        return Response(output = out.value.getByteArray(0, outLength.value).encodeBase64())
    }

}