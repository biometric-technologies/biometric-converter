package net.iriscan.bcws.extension

import org.apache.commons.codec.binary.Base64

/**
 * @author Slava Gornostal
 */
fun String.decodeBase64(): ByteArray = Base64.decodeBase64(this)

fun ByteArray.encodeBase64(): String = Base64.encodeBase64String(this)