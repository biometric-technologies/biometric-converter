package net.iriscan.bcws.lib

import com.sun.jna.Library
import com.sun.jna.ptr.IntByReference
import com.sun.jna.ptr.PointerByReference

/**
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
}
