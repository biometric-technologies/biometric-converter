package net.iriscan.bcws.dto

/**
 * @author Slava Gornostal
 */
data class Response(val output: String)
data class BatchResponse(val id: String, val output: String)

data class BatchResponseList(val data: List<BatchResponse>)
