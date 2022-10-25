package net.iriscan.bcws.dto

import net.iriscan.bcws.lib.FileFormat

/**
 * @author Slava Gornostal
 */
data class Request(
    val input: String,
    val inputType: FileFormat,
    val outputType: FileFormat,
    val imageResX: Int = 0,
    val imageResY: Int = 0
)

data class BatchRequest(
    val id: String,
    val input: String,
    val inputType: FileFormat,
    val outputType: FileFormat,
    val imageResX: Int = 0,
    val imageResY: Int = 0
)

data class BatchRequestList(val data: List<BatchRequest>)