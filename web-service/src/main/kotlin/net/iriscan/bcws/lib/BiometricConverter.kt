package net.iriscan.bcws.lib

import com.sun.jna.Library
import com.sun.jna.ptr.IntByReference
import com.sun.jna.ptr.PointerByReference

/**
 * @author Slava Gornostal
 */
interface BiometricConverter : Library {
    fun detectEye(
        input: ByteArray,
        inputLength: Int,
        outputType: String,
        output: PointerByReference,
        outputLength: IntByReference
    ): Int
}