package net.iriscan.bcws.lib

import com.sun.jna.Library
import com.sun.jna.Pointer
import com.sun.jna.ptr.IntByReference
import com.sun.jna.ptr.PointerByReference

/**
 * Binding of libconverter (include/converter.h). The conversion functions
 * return 0 on success and -1 on error; converter_last_error() then gives the
 * reason. Output buffers are released with converter_free().
 *
 * @author Slava Gornostal
 */
interface Converter : Library {
    fun img2fmr(
        input: ByteArray,
        inputLength: Int,
        outputType: String,
        output: PointerByReference,
        outputLength: IntByReference
    ): Int

    fun fmr2fmr(
        input: ByteArray,
        inputLength: Int,
        output: PointerByReference,
        outputLength: IntByReference,
        inputType: String,
        outputType: String,
    ): Int

    fun fmr2fmr_iso_card(
        input: ByteArray,
        inputLength: Int,
        output: PointerByReference,
        outputLength: IntByReference,
        inputType: String,
        outputType: String,
        imageResX: Int,
        imageResY: Int,
    ): Int

    fun converter_last_error(): String

    fun converter_free(data: Pointer?)
}
