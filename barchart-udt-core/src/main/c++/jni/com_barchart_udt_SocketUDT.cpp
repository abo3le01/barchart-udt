/**
 * =================================================================================
 *
 * BSD LICENCE (http://en.wikipedia.org/wiki/BSD_licenses)
 *
 * Copyright (C) 2009-2012, Barchart, Inc. (http://www.barchart.com/)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *     * Neither the name of the Barchart, Inc. nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Developers: Andrei Pozolotin;
 *
 * =================================================================================
 */
/*
 * NOTE: provides ONLY ipv4 implementation (not ipv6)
 *
 */

// ### keep outside [extern "C"]
#include "com_barchart_udt_SocketUDT.h"
#include "JNICCCFactory.h"
#include "JNICCC.h"

#include "udt.h"

#include "JNIHelpers.h"

// do not use cout; else will introduce GLIBCXX_3.4.9 dependency with 'g++ -O2'
//#include <iostream>

using namespace std;

// do not use to avoid ambiguity
//using namespace UDT;

// for JNI method signature compatibility
extern "C" { /* specify the C calling convention */

// ### keep inside extern "C"

// ########################################################

// ### UDT references

// UDT classes
static jclass udt_clsSocketUDT; // com.barchart.udt.SocketUDT
static jclass udt_clsTypeUDT; // com.barchart.udt.TypeUDT
static jclass udt_clsFactoryUDT; // com.barchart.udt.FactoryUDT
static jclass udt_clsMonitorUDT; // com.barchart.udt.MonitorUDT
static jclass udt_clsExceptionUDT; // com.barchart.udt.ExceptionUDT
static jclass udt_clsLingerUDT; // com.barchart.udt.ExceptionUDT
static jclass udt_clsCCC; // com.barchart.udt.CCC

// UDT fields: com.barchart.udt.SocketUDT
static jfieldID udts_TypeID;
static jfieldID udts_RemoteSocketAddressID;
static jfieldID udts_LocalSocketAddressID;
static jfieldID udts_MonitorID;
static jfieldID udts_SocketID;
static jfieldID udts_IsSelectedReadID;
static jfieldID udts_IsSelectedWriteID;
static jfieldID udts_IsSelectedExceptionID;

// UDT fields: com.barchart.udt.TypeUDT
static jfieldID udtt_TypeCodeID;

// UDT methods
static jmethodID udt_clsSocketUDT_initID1; // new SocketUDT(int type, int id)
static jmethodID udt_clsExceptionUDT_initID0; // new ExceptionUDT(int code, String message)
static jmethodID udt_clsLingerUDT_initID; // new LingerUDT(int value)

// UDT fields: com.barchart.udt.MonitorUDT
//
// global measurements
static jfieldID udtm_msTimeStamp; // time since the UDT entity is started, in milliseconds
static jfieldID udtm_pktSentTotal; // total number of sent data packets, including retransmissions
static jfieldID udtm_pktRecvTotal; // total number of received packets
static jfieldID udtm_pktSndLossTotal; // total number of lost packets (sender side)
static jfieldID udtm_pktRcvLossTotal; // total number of lost packets (receiver side)
static jfieldID udtm_pktRetransTotal; // total number of retransmitted packets
static jfieldID udtm_pktSentACKTotal; // total number of sent ACK packets
static jfieldID udtm_pktRecvACKTotal; // total number of received ACK packets
static jfieldID udtm_pktSentNAKTotal; // total number of sent NAK packets
static jfieldID udtm_pktRecvNAKTotal; // total number of received NAK packets
static jfieldID udtm_usSndDurationTotal; // total time duration when UDT is sending data (idle time exclusive)
//
// local measurements
static jfieldID udtm_pktSent; // number of sent data packets, including retransmissions
static jfieldID udtm_pktRecv; // number of received packets
static jfieldID udtm_pktSndLoss; // number of lost packets (sender side)
static jfieldID udtm_pktRcvLoss; // number of lost packets (receiverer side)
static jfieldID udtm_pktRetrans; // number of retransmitted packets
static jfieldID udtm_pktSentACK; // number of sent ACK packets
static jfieldID udtm_pktRecvACK; // number of received ACK packets
static jfieldID udtm_pktSentNAK; // number of sent NAK packets
static jfieldID udtm_pktRecvNAK; // number of received NAK packets
static jfieldID udtm_mbpsSendRate; // sending rate in Mb/s
static jfieldID udtm_mbpsRecvRate; // receiving rate in Mb/s
static jfieldID udtm_usSndDuration; // busy sending time (i.e., idle time exclusive)
//
// instant measurements
static jfieldID udtm_usPktSndPeriod; // packet sending period, in microseconds
static jfieldID udtm_pktFlowWindow; // flow window size, in number of packets
static jfieldID udtm_pktCongestionWindow; // congestion window size, in number of packets
static jfieldID udtm_pktFlightSize; // number of packets on flight
static jfieldID udtm_msRTT; // RTT, in milliseconds
static jfieldID udtm_mbpsBandwidth; // estimated bandwidth, in Mb/s
static jfieldID udtm_byteAvailSndBuf; // available UDT sender buffer size
static jfieldID udtm_byteAvailRcvBuf; // available UDT receiver buffer size

// ########################################################

// not used
void XXX_ThrowSocketExceptionMessage( //
		JNIEnv * const env, //
		const char* message //
		) {
	CHK_NUL_RET_RET(env, "env");
	CHK_NUL_RET_RET(message, "message");
	CHK_NUL_RET_RET(jdk_clsSocketException, "jdk_clsSocketException");
	env->ThrowNew(jdk_clsSocketException, message);
}

// ########################################################

// custom class for struct linger conversion
jobject UDT_NewLingerUDT( //
		JNIEnv * const env, //
		linger * const lingerValue //
		) {
	CHK_NUL_RET_NUL(env, "env");
	CHK_NUL_RET_NUL(lingerValue, "lingerValue");
	int value;
	if (lingerValue->l_onoff == 0) {
		value = 0;
	} else {
		value = lingerValue->l_linger;
	}
	jobject objLinger = env->NewObject(udt_clsLingerUDT,
			udt_clsLingerUDT_initID, (jint) value);
	CHK_NUL_RET_NUL(objLinger, "objLinger");
	return objLinger;
}

// java wrapper exception error codes; keep in sync with ErrorUDT.java
#define UDT_WRAPPER_UNKNOWN -1 // WRAPPER_UNKNOWN(-1, "unknown error code"), //
#define UDT_WRAPPER_UNIMPLEMENTED -2 // WRAPPER_UNIMPLEMENTED(-2, "this feature is not yet implemented"), //
#define UDT_WRAPPER_MESSAGE -3 // WRAPPER_MESSAGE(-3, "wrapper generated error"), //
#define UDT_USER_DEFINED_MESSAGE -4 // USER_DEFINED_MESSAGE(-4, "user defined message"), //
//

// NOTE: socket id stored in java object SocketUDT
jint UDT_GetSocketID( //
		JNIEnv * const env, //
		const jobject self //
		) {
	return env->GetIntField(self, udts_SocketID);
}
// NOTE: socket id stored in java object SocketUDT
void UDT_SetSocketID( //
		JNIEnv * const env, //
		const jobject self, //
		const jint socketID //
		) {
	env->SetIntField(self, udts_SocketID, socketID);
}

jthrowable UDT_NewExceptionUDT( //
		JNIEnv * const env, //
		const jint socketID, //
		const jint errorCode, //
		const char* message //
		) {
	CHK_NUL_RET_NUL(env, "env");
	const jstring messageString = env->NewStringUTF(message);
	CHK_NUL_RET_NUL(messageString, "messageString");
	const jobject exception = env->NewObject(udt_clsExceptionUDT,
			udt_clsExceptionUDT_initID0, socketID, errorCode, messageString);
	return static_cast<jthrowable>(exception);
}

void UDT_ThrowExceptionUDT_Message( //
		JNIEnv * const env, //
		const jint socketID, //
		const char *comment //
		) {
	CHK_NUL_RET_RET(env, "env");
	CHK_NUL_RET_RET(comment, "comment");
	const jint code = UDT_WRAPPER_MESSAGE;
	const jthrowable exception = UDT_NewExceptionUDT(env, socketID, code,
			comment);
	CHK_NUL_RET_RET(exception, "exception");
	env->Throw(exception);
}

// socketID == 0 means not applicable / not known id
void UDT_ThrowExceptionUDT_ErrorInfo( //
		JNIEnv * const env, //
		const jint socketID, //
		const char* comment, //
		UDT::ERRORINFO * const errorInfo //
		) {
	CHK_NUL_RET_RET(env, "env");
	CHK_NUL_RET_RET(comment, "comment");
	CHK_NUL_RET_RET(errorInfo, "errorInfo");
	const jint code = errorInfo->getErrorCode();
	const jthrowable exception = //
			UDT_NewExceptionUDT(env, socketID, code, comment);
	CHK_NUL_RET_RET(exception, "exception");
	env->Throw(exception);
}

void UDT_InitFieldMonitor( //
		JNIEnv * const env //
		) {

	const jclass cls = udt_clsMonitorUDT;

	// global measurements
	udtm_msTimeStamp = env->GetFieldID(cls, "msTimeStamp", "J"); // time since the UDT entity is started, in milliseconds
	udtm_pktSentTotal = env->GetFieldID(cls, "pktSentTotal", "J"); // total number of sent data packets, including retransmissions
	udtm_pktRecvTotal = env->GetFieldID(cls, "pktRecvTotal", "J"); // total number of received packets
	udtm_pktSndLossTotal = env->GetFieldID(cls, "pktSndLossTotal", "I"); // total number of lost packets (sender side)
	udtm_pktRcvLossTotal = env->GetFieldID(cls, "pktRcvLossTotal", "I"); // total number of lost packets (receiver side)
	udtm_pktRetransTotal = env->GetFieldID(cls, "pktRetransTotal", "I"); // total number of retransmitted packets
	udtm_pktSentACKTotal = env->GetFieldID(cls, "pktSentACKTotal", "I"); // total number of sent ACK packets
	udtm_pktRecvACKTotal = env->GetFieldID(cls, "pktRecvACKTotal", "I"); // total number of received ACK packets
	udtm_pktSentNAKTotal = env->GetFieldID(cls, "pktSentNAKTotal", "I"); // total number of sent NAK packets
	udtm_pktRecvNAKTotal = env->GetFieldID(cls, "pktRecvNAKTotal", "I"); // total number of received NAK packets
	udtm_usSndDurationTotal = env->GetFieldID(cls, "usSndDurationTotal", "J"); // total time duration when UDT is sending data (idle time exclusive)

	// local measurements
	udtm_pktSent = env->GetFieldID(cls, "pktSent", "J"); // number of sent data packets, including retransmissions
	udtm_pktRecv = env->GetFieldID(cls, "pktRecv", "J"); // number of received packets
	udtm_pktSndLoss = env->GetFieldID(cls, "pktSndLoss", "I"); // number of lost packets (sender side)
	udtm_pktRcvLoss = env->GetFieldID(cls, "pktRcvLoss", "I"); // number of lost packets (receiverer side)
	udtm_pktRetrans = env->GetFieldID(cls, "pktRetrans", "I"); // number of retransmitted packets
	udtm_pktSentACK = env->GetFieldID(cls, "pktSentACK", "I"); // number of sent ACK packets
	udtm_pktRecvACK = env->GetFieldID(cls, "pktRecvACK", "I"); // number of received ACK packets
	udtm_pktSentNAK = env->GetFieldID(cls, "pktSentNAK", "I"); // number of sent NAK packets
	udtm_pktRecvNAK = env->GetFieldID(cls, "pktRecvNAK", "I"); // number of received NAK packets
	udtm_mbpsSendRate = env->GetFieldID(cls, "mbpsSendRate", "D"); // sending rate in Mb/s
	udtm_mbpsRecvRate = env->GetFieldID(cls, "mbpsRecvRate", "D"); // receiving rate in Mb/s
	udtm_usSndDuration = env->GetFieldID(cls, "usSndDuration", "J"); // busy sending time (i.e., idle time exclusive)

	// instant measurements
	udtm_usPktSndPeriod = env->GetFieldID(cls, "usPktSndPeriod", "D"); // packet sending period, in microseconds
	udtm_pktFlowWindow = env->GetFieldID(cls, "pktFlowWindow", "I"); // flow window size, in number of packets
	udtm_pktCongestionWindow = env->GetFieldID(cls, "pktCongestionWindow", "I"); // congestion window size, in number of packets
	udtm_pktFlightSize = env->GetFieldID(cls, "pktFlightSize", "I"); // number of packets on flight
	udtm_msRTT = env->GetFieldID(cls, "msRTT", "D"); // RTT, in milliseconds
	udtm_mbpsBandwidth = env->GetFieldID(cls, "mbpsBandwidth", "D"); // estimated bandwidth, in Mb/s
	udtm_byteAvailSndBuf = env->GetFieldID(cls, "byteAvailSndBuf", "I"); // available UDT sender buffer size
	udtm_byteAvailRcvBuf = env->GetFieldID(cls, "byteAvailRcvBuf", "I"); // available UDT receiver buffer size

}

void UDT_InitClassRefAll(JNIEnv * const env) {

	// JDK

	X_InitClassReference(env, &jdk_clsBoolean, "java/lang/Boolean");
	X_InitClassReference(env, &jdk_clsInteger, "java/lang/Integer");
	X_InitClassReference(env, &jdk_clsLong, "java/lang/Long");

	X_InitClassReference(env, &jdk_clsInet4Address, "java/net/Inet4Address");
	X_InitClassReference(env, &jdk_clsInet6Address, "java/net/Inet6Address");
	X_InitClassReference(env, &jdk_clsInetSocketAddress,
			"java/net/InetSocketAddress");

	X_InitClassReference(env, &jdk_clsSocketException,
			"java/net/SocketException");

	X_InitClassReference(env, &jdk_clsSet, "java/util/Set");
	X_InitClassReference(env, &jdk_clsIterator, "java/util/Iterator");

	// UDT

	X_InitClassReference(env, &udt_clsSocketUDT, //
			"com/barchart/udt/SocketUDT");
	X_InitClassReference(env, &udt_clsTypeUDT, //
			"com/barchart/udt/TypeUDT");
	X_InitClassReference(env, &udt_clsFactoryUDT, //
			"com/barchart/udt/FactoryUDT");
	X_InitClassReference(env, &udt_clsMonitorUDT, //
			"com/barchart/udt/MonitorUDT");
	X_InitClassReference(env, &udt_clsExceptionUDT, //
			"com/barchart/udt/ExceptionUDT");
	X_InitClassReference(env, &udt_clsLingerUDT, //
			"com/barchart/udt/LingerUDT");
	X_InitClassReference(env, &udt_clsCCC, //
			"com/barchart/udt/CCC");
	X_InitClassReference(env, &udt_clsFactoryInterfaceUDT,
			"com/barchart/udt/FactoryInterfaceUDT");

}

void UDT_FreeClassRefAll(JNIEnv * const env) {

	// JDK

	X_FreeClassReference(env, &jdk_clsBoolean);
	X_FreeClassReference(env, &jdk_clsInteger);
	X_FreeClassReference(env, &jdk_clsLong);

	X_FreeClassReference(env, &jdk_clsInet4Address);
	X_FreeClassReference(env, &jdk_clsInet6Address);
	X_FreeClassReference(env, &jdk_clsInetSocketAddress);

	X_FreeClassReference(env, &jdk_clsSocketException);

	X_FreeClassReference(env, &jdk_clsSet);
	X_FreeClassReference(env, &jdk_clsIterator);

	// UDT

	X_FreeClassReference(env, &udt_clsSocketUDT);
	X_FreeClassReference(env, &udt_clsTypeUDT);
	X_FreeClassReference(env, &udt_clsFactoryUDT);
	X_FreeClassReference(env, &udt_clsMonitorUDT);
	X_FreeClassReference(env, &udt_clsExceptionUDT);
	X_FreeClassReference(env, &udt_clsLingerUDT);
	X_FreeClassReference(env, &udt_clsCCC);
	X_FreeClassReference(env, &udt_clsFactoryInterfaceUDT);
}

void UDT_InitFieldRefAll(JNIEnv * const env) {

	// JDK

	ia_AddressID = env->GetFieldID(jdk_clsInet4Address, "address", "I");
	isa_InetAddressID = env->GetFieldID(jdk_clsInetSocketAddress, "addr",
			"Ljava/net/InetAddress;");
	isa_PortID = env->GetFieldID(jdk_clsInetSocketAddress, "port", "I");

	// UDT SocketUDT

	udts_SocketID = env->GetFieldID(udt_clsSocketUDT, //
			"socketID", "I");
	udts_RemoteSocketAddressID = env->GetFieldID(udt_clsSocketUDT, //
			"remoteSocketAddress", "Ljava/net/InetSocketAddress;");
	udts_LocalSocketAddressID = env->GetFieldID(udt_clsSocketUDT, //
			"localSocketAddress", "Ljava/net/InetSocketAddress;");
	udts_TypeID = env->GetFieldID(udt_clsSocketUDT, //
			"type", "Lcom/barchart/udt/TypeUDT;");
	udts_MonitorID = env->GetFieldID(udt_clsSocketUDT, //
			"monitor", "Lcom/barchart/udt/MonitorUDT;");

	udts_IsSelectedReadID = env->GetFieldID(udt_clsSocketUDT, //
			"isSelectedRead", "Z");
	udts_IsSelectedWriteID = env->GetFieldID(udt_clsSocketUDT, //
			"isSelectedWrite", "Z");
	udts_IsSelectedExceptionID = env->GetFieldID(udt_clsSocketUDT, //
			"isSelectedException", "Z");

	// UDT TypeUDT

	udtt_TypeCodeID = env->GetFieldID(udt_clsTypeUDT, "code", "I");

	// UDT MonitorUDT

	UDT_InitFieldMonitor(env);

	// UDT CCC

	udt_clsCCC_fld_nativeHandleID = env->GetFieldID(udt_clsCCC, "nativeHandle",
			"J");

}

void X_InitMethodRef( //
		JNIEnv * const env, //
		jmethodID *methodID, //
		const jclass klaz, //
		const char* name, //
		const char* signature //
		) {

	*methodID = env->GetMethodID(klaz, name, signature);

	CHK_NUL_RET_RET(*methodID, name);

}

void UDT_InitMethodRefAll( //
		JNIEnv * const env //
		) {

	// JDK

	jdk_clsBoolean_initID = env->GetMethodID(jdk_clsBoolean, //
			"<init>", "(Z)V");
	CHK_NUL_RET_RET(jdk_clsBoolean_initID, "jdk_clsBoolean_initID");

	jdk_clsInteger_initID = env->GetMethodID(jdk_clsInteger, //
			"<init>", "(I)V");
	CHK_NUL_RET_RET(jdk_clsInteger_initID, "jdk_clsInteger_initID");

	jdk_clsLong_initID = env->GetMethodID(jdk_clsLong, //
			"<init>", "(J)V");
	CHK_NUL_RET_RET(jdk_clsLong_initID, "jdk_clsLong_initID");

	jdk_clsInet4Address_initID = env->GetMethodID(jdk_clsInet4Address, "<init>",
			"()V");
	CHK_NUL_RET_RET(jdk_clsInet4Address_initID, "jdk_clsInet4Address_initID");

	jdk_clsInetSocketAddress_initID = env->GetMethodID(jdk_clsInetSocketAddress, //
			"<init>", "(Ljava/net/InetAddress;I)V");
	CHK_NUL_RET_RET(jdk_clsInetSocketAddress_initID,
			"jdk_clsInetSocketAddress_initID");

	// java.util.Set
	jdk_clsSet_iteratorID = env->GetMethodID(jdk_clsSet, //
			"iterator", "()Ljava/util/Iterator;");
	CHK_NUL_RET_RET(jdk_clsSet_iteratorID, "jdk_clsSet_iteratorID");
	jdk_clsSet_addID = env->GetMethodID(jdk_clsSet, //
			"add", "(Ljava/lang/Object;)Z");
	CHK_NUL_RET_RET(jdk_clsSet_addID, "jdk_clsSet_addID");
	jdk_clsSet_containsID = env->GetMethodID(jdk_clsSet, //
			"contains", "(Ljava/lang/Object;)Z");
	CHK_NUL_RET_RET(jdk_clsSet_containsID, "jdk_clsSet_containsID");

	// java.util.Iterator
	jdk_clsIterator_hasNextID = env->GetMethodID(jdk_clsIterator, //
			"hasNext", "()Z");
	CHK_NUL_RET_RET(jdk_clsIterator_hasNextID, "jdk_clsIterator_hasNextID");
	jdk_clsIterator_nextID = env->GetMethodID(jdk_clsIterator, //
			"next", "()Ljava/lang/Object;");
	CHK_NUL_RET_RET(jdk_clsIterator_nextID, "jdk_clsIterator_nextID");

	// UDT

	udt_clsSocketUDT_initID1 = env->GetMethodID(udt_clsSocketUDT, //
			"<init>", "(Lcom/barchart/udt/TypeUDT;I)V");
	CHK_NUL_RET_RET(udt_clsSocketUDT_initID1, "udt_clsSocketUDT_initID1");

	udt_clsExceptionUDT_initID0 = env->GetMethodID(udt_clsExceptionUDT, //
			"<init>", "(IILjava/lang/String;)V");
	CHK_NUL_RET_RET(udt_clsExceptionUDT_initID0, "udt_clsExceptionUDT_initID0");

	udt_clsLingerUDT_initID = env->GetMethodID(udt_clsLingerUDT, //
			"<init>", "(I)V");
	CHK_NUL_RET_RET(udt_clsLingerUDT_initID, "udt_clsLingerUDT_initID");

}

// ########################################################

/* signature is a monotonously increasing integer; set in java class SocketUDT */

// validate consistency of java code and native library
JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_getSignatureJNI0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT //
		) {

	UNUSED(env);
	UNUSED(clsSocketUDT);

	return com_barchart_udt_SocketUDT_SIGNATURE_JNI;

}

// called on java class load
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_initClass0( //
		JNIEnv* const env, //
		const jclass clsSocketUDT //
		) {

	UNUSED(env);
	UNUSED(clsSocketUDT);

	UDT_InitClassRefAll(env);

	UDT_InitFieldRefAll(env);

	UDT_InitMethodRefAll(env);

	const int rv = UDT::startup();

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, 0, "initClass0:startup", &errorInfo);
		return;
	}

}

// called on java class unload
void JNICALL Java_com_barchart_udt_SocketUDT_stopClass0( //
		JNIEnv* const env, //
		const jclass clsSocketUDT //
		) {

	UNUSED(env);
	UNUSED(clsSocketUDT);

	//Release global JNI references
	UDT_FreeClassRefAll(env);

	const int rv = UDT::cleanup();

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, 0, "stopClass0:cleanup", &errorInfo);
		return;
	}

}

// used by default constructor
JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_initInstance0( //
		JNIEnv * const env, //
		const jobject self, //
		const jint typeCode //
		) {

	int socketAddressFamily = AF_INET;
	int socketType = typeCode;

	const jint socketID = UDT::socket(socketAddressFamily, socketType, 0);

	UDT_SetSocketID(env, self, socketID);

	if (socketID == UDT::INVALID_SOCK) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "initInstance0:INVALID_SOCK", &errorInfo);
		return JNI_ERR;
	}

	return socketID;

}

// used by accept constructor
JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_initInstance1( //
		JNIEnv * const env, //
		const jobject self, //
		const jint socketID //
		) {

	UNUSED(self);

	if (socketID == UDT::INVALID_SOCK) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "initInstance1:INVALID_SOCK", &errorInfo);
		return JNI_ERR;
	}

	return socketID;

}

JNIEXPORT jobject JNICALL Java_com_barchart_udt_SocketUDT_accept0( //
		JNIEnv * const env, //
		const jobject self //
		) {

	sockaddr remoteSockAddr;
	int remoteSockAddrSize = sizeof(remoteSockAddr);

	const jint socketID = UDT_GetSocketID(env, self);

	const jint socketACC = UDT::accept(socketID, &remoteSockAddr,
			&remoteSockAddrSize);

	if (socketACC == UDT::INVALID_SOCK) {

		UDT::ERRORINFO errorInfo = UDT::getlasterror();

		const jint errorCode = errorInfo.getErrorCode();

		if (errorCode == UDT::ERRORINFO::EASYNCRCV) {
			// not a java exception: non-blocking mode return, when not connections in the queue
		} else {
			// really exception
			UDT_ThrowExceptionUDT_ErrorInfo( //
					env, socketID, "accept0:accept", &errorInfo);
		}

		return NULL;

	}

	jobject objTypeUDT = env->GetObjectField(self, udts_TypeID);

	CHK_NUL_RET_NUL(objTypeUDT, "objTypeUDT");

	jobject objSocketUDT = env->NewObject(udt_clsSocketUDT,
			udt_clsSocketUDT_initID1, objTypeUDT, socketACC);

	CHK_NUL_RET_NUL(objSocketUDT, "objSocketUDT");

	return objSocketUDT;

}

JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_bind0( //
		JNIEnv * const env, //
		const jobject self, //
		const jobject objLocalSocketAddress //
		) {

	const jint socketID = UDT_GetSocketID(env, self);

	int rv;

	sockaddr localSockAddr;

	rv = X_InitSockAddr(&localSockAddr);

	if (rv == JNI_ERR) {
		UDT_ThrowExceptionUDT_Message(env, socketID, "can not X_InitSockAddr");
		return;
	}

	rv = X_ConvertInetSocketAddressToSockaddr(env, objLocalSocketAddress,
			&localSockAddr);

	if (rv == JNI_ERR) {
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"can not X_ConvertInetSocketAddressToSockaddr");
		return;
	}

	rv = UDT::bind(socketID, &localSockAddr, sizeof(localSockAddr));

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "bind0:bind", &errorInfo);
		return;
	}

}

JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_close0( //
		JNIEnv * const env, //
		const jobject self //
		) {

	const jint socketID = UDT_GetSocketID(env, self);

	const int rv = UDT::close(socketID);

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "close0:close", &errorInfo);
		return;
	}

}

//
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_connect0( //
		JNIEnv * const env, const jobject self, //
		const jobject objRemoteSocketAddress //
		) {

	const jint socketID = UDT_GetSocketID(env, self);

	if (objRemoteSocketAddress == NULL) {
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"objRemoteSocketAddress == NULL");
		return;
	}

	int rv;

	sockaddr remoteSockAddr;

	rv = X_InitSockAddr(&remoteSockAddr);

	if (rv == JNI_ERR) {
		UDT_ThrowExceptionUDT_Message(env, socketID, "can not X_InitSockAddr");
		return;
	}

	rv = X_ConvertInetSocketAddressToSockaddr(env, //
			objRemoteSocketAddress, &remoteSockAddr);

	if (rv == JNI_ERR) {
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"can not X_ConvertInetSocketAddressToSockaddr");
		return;
	}

	rv = UDT::connect(socketID, &remoteSockAddr, sizeof(remoteSockAddr));

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "connect0:connect", &errorInfo);
		return;
	}

}

JNIEXPORT jboolean JNICALL Java_com_barchart_udt_SocketUDT_hasLoadedRemoteSocketAddress( //
		JNIEnv * const env, //
		const jobject self //
		) {

	const jint socketID = UDT_GetSocketID(env, self);

	sockaddr remoteSockAddr;
	int remoteSockAddrSize = sizeof(remoteSockAddr);

	// "peer" is remote
	const int rv = UDT::getpeername( //
			socketID, &remoteSockAddr, &remoteSockAddrSize);

	if (rv == UDT::ERROR) {
		// no exceptions
		return JNI_FALSE;
	}

	jobject objRemoteSocketAddress = env->GetObjectField(self,
			udts_RemoteSocketAddressID);

	if (objRemoteSocketAddress == NULL
			|| !X_IsSockaddrEqualsInetSocketAddress(env, &remoteSockAddr,
					objRemoteSocketAddress)) {

		objRemoteSocketAddress = X_NewInetSocketAddress(env, &remoteSockAddr);

		env->SetObjectField(self, udts_RemoteSocketAddressID,
				objRemoteSocketAddress);

	}

	return JNI_TRUE;

}

JNIEXPORT jboolean JNICALL Java_com_barchart_udt_SocketUDT_hasLoadedLocalSocketAddress( //
		JNIEnv * const env, //
		const jobject self //
		) {

	const jint socketID = UDT_GetSocketID(env, self);

	sockaddr localSockAddr;
	int localSockAddrSize = sizeof(localSockAddr);

	// "sock" is local
	const int rv = UDT::getsockname( //
			socketID, &localSockAddr, &localSockAddrSize);

	if (rv == UDT::ERROR) {
		// no exceptions
		return JNI_FALSE;
	}

	jobject objLocalSocketAddress = env->GetObjectField(self,
			udts_LocalSocketAddressID);

	if (objLocalSocketAddress == NULL
			|| !X_IsSockaddrEqualsInetSocketAddress(env, &localSockAddr,
					objLocalSocketAddress)) {

		objLocalSocketAddress = X_NewInetSocketAddress(env, &localSockAddr);

		env->SetObjectField(self, udts_LocalSocketAddressID,
				objLocalSocketAddress);

	}

	return JNI_TRUE;

}

// struct that fits all UDT options types
union UDT_OptVal {
	void* factory;
	linger lingerValue;
	int64_t longValue;
	int intValue;
	bool boolValue;
};

JNIEXPORT jobject JNICALL Java_com_barchart_udt_SocketUDT_getOption0( //
		JNIEnv * const env, //
		const jobject self, //
		const jint enumCode, //
		const jclass klaz //
		) {

	UDT::SOCKOPT optionName = (UDT::SOCKOPT) enumCode;
	UDT_OptVal optionValue;
	int optionValueSize = sizeof(optionValue);

	const jint socketID = UDT_GetSocketID(env, self);

	const int rv = UDT::getsockopt( //
			socketID, 0, optionName, (void*) &optionValue, &optionValueSize);

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "getOption0:getsockopt", &errorInfo);
		return NULL;
	}

	if (env->IsSameObject(klaz, jdk_clsBoolean)) {

		return X_NewBoolean(env, optionValue.boolValue);

	} else if (env->IsSameObject(klaz, jdk_clsInteger)) {

		return X_NewInteger(env, optionValue.intValue);

	} else if (env->IsSameObject(klaz, udt_clsLingerUDT)) {

		return UDT_NewLingerUDT(env, &(optionValue.lingerValue));

	} else if (env->IsSameObject(klaz, jdk_clsLong)) {

		return X_NewLong(env, optionValue.longValue);

	} else if (env->IsSameObject(klaz, udt_clsFactoryUDT)) {

		CCC* pCCC = reinterpret_cast<CCC*>(optionValue.factory);

		//check to see if they type is as JNICCC class
		JNICCC* pJNICCC = dynamic_cast<JNICCC*>(pCCC);

		if (pJNICCC != NULL) {
			//it is, so return the corresponding Java CCC (or derivative) class
			return pJNICCC->getJavaCCC();
		} else {
			//it's a plain CCC class or standard C++ congestion
			//control class so return NULL instead
			return NULL;
		}

	} else {

		UDT_ThrowExceptionUDT_Message(env, socketID,
				"unsupported option class in OptionUDT");

		return NULL;
	}

}

JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_setOption0( //
		JNIEnv * const env, //
		const jobject self, //
		const jint enumCode, //
		const jclass klaz, //
		const jobject objValue //
		) {

	const jint socketID = UDT_GetSocketID(env, self);

	UDT::SOCKOPT optionName = (UDT::SOCKOPT) enumCode;
	UDT_OptVal optionValue;
	int optionValueSize = sizeof(optionValue);

	if (env->IsSameObject(klaz, jdk_clsBoolean)) {

		jmethodID methodID = //
				env->GetMethodID(jdk_clsBoolean, "booleanValue", "()Z");
		jboolean value = env->CallBooleanMethod(objValue, methodID);

		optionValue.boolValue = BOOL(value);
		optionValueSize = sizeof(bool);

	} else if (env->IsSameObject(klaz, jdk_clsInteger)) {

		jmethodID methodID = //
				env->GetMethodID(jdk_clsInteger, "intValue", "()I");
		jint value = env->CallIntMethod(objValue, methodID);

		optionValue.intValue = value;
		optionValueSize = sizeof(int);

	} else if (env->IsSameObject(klaz, udt_clsLingerUDT)) {

		jmethodID methodID = //
				env->GetMethodID(udt_clsLingerUDT, "intValue", "()I");
		int value = env->CallIntMethod(objValue, methodID);

		if (value <= 0) {
			optionValue.lingerValue.l_onoff = 0;
			optionValue.lingerValue.l_linger = 0;
		} else {
			optionValue.lingerValue.l_onoff = 1;
			optionValue.lingerValue.l_linger = value; // msvc C4244
		}

		optionValueSize = sizeof(linger);

	} else if (env->IsSameObject(klaz, jdk_clsLong)) {

		jmethodID methodID = //
				env->GetMethodID(jdk_clsLong, "longValue", "()J");
		jlong value = env->CallLongMethod(objValue, methodID);

		optionValue.longValue = value;
		optionValueSize = sizeof(int64_t);

	} else if (env->IsSameObject(klaz, udt_clsFactoryUDT)) {

		optionValue.factory = new JNICCCFactory(env, objValue);
		optionValueSize = sizeof(void*);

	} else {

		UDT_ThrowExceptionUDT_Message(env, socketID,
				"unsupported option class in OptionUDT");

		return;
	}

	const int rv = UDT::setsockopt(socketID, 0, optionName,
			(void*) (optionName == UDT_CC ? optionValue.factory : &optionValue),
			optionValueSize);

	if (optionName == UDT_CC && optionValue.factory != NULL) {
		delete reinterpret_cast<JNICCCFactory*>(optionValue.factory);
	}

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "setOption0:setsockopt", &errorInfo);
		return;
	}

}

JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_listen0( //
		JNIEnv * const env, //
		const jobject self, //
		const jint queueSize //
		) {

	const jint socketID = UDT_GetSocketID(env, self);

	const int rv = UDT::listen(socketID, queueSize);

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "listen0:listen", &errorInfo);
		return;
	}

}

// #########################################################################

bool X_IsValidRange(JNIEnv *env, jint socketID, //
		jlong position, jlong limit, jlong capacity) {
	if (!X_IsInRange(0, position, capacity)) {
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"position is out of range");
		return false;
	}
	if (!X_IsInRange(0, limit, capacity)) {
		UDT_ThrowExceptionUDT_Message(env, socketID, "limit is out of range");
		return false;
	}
	if (position > limit) {
		UDT_ThrowExceptionUDT_Message(env, socketID, "position > limit");
		return false;
	}
	return true;
}

// #######################################################################

/*
 * receiveX call is shared for both receive() and receivemsg()
 */

// return values, if exception is NOT thrown
// -1 : nothing received (non-blocking only)
// =0 : timeout expired (blocking only)
// >0 : normal receive
jint UDT_ReturnReceiveError( //
		JNIEnv * const env, //
		const jint socketID //
		) {
	UDT::ERRORINFO errorInfo = UDT::getlasterror();
	const int errorCode = errorInfo.getErrorCode();
	if (errorCode == UDT::ERRORINFO::EASYNCRCV) {
		// not a java exception: non-blocking mode return when nothing is received
	} else {
		// really exception
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "recv/recvmsg", &errorInfo);
	}
	return JNI_ERR;
}

// receive into complete array
JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_receive0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint socketID, //
		const jint socketType, //
		const jbyteArray arrayObj //
		) {

	UNUSED(clsSocketUDT);

	jboolean isCopy; // whether JVM returns a reference or a copy

	jbyte * const data = env->GetByteArrayElements(arrayObj, &isCopy); // note: must release

	const jsize size = env->GetArrayLength(arrayObj);

	int rv;

	switch (socketType) {
	case SOCK_STREAM:
		rv = UDT::recv(socketID, (char*) data, (int) size, 0);
		break;
	case SOCK_DGRAM:
		rv = UDT::recvmsg(socketID, (char*) data, (int) size);
		break;
	default:
		env->ReleaseByteArrayElements(arrayObj, data, JNI_ABORT); // do not copy back
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"recv/recvmsg : unexpected socketType");
		return JNI_ERR;
	}

	if (rv > 0) { // normal
		env->ReleaseByteArrayElements(arrayObj, data, JNI_UPDATE); //
		return rv;
	} else if (rv < 0) { // UDT::ERROR
		env->ReleaseByteArrayElements(arrayObj, data, JNI_ABORT); //
		return UDT_ReturnReceiveError(env, socketID);
	} else { // ==0; UDT_TIMEOUT
		env->ReleaseByteArrayElements(arrayObj, data, JNI_ABORT); //
		return UDT_TIMEOUT;
	}

}

// receive into a portion of an array
JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_receive1( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint socketID, //
		const jint socketType, //
		const jbyteArray arrayObj, //
		const jint position, //
		const jint limit //
		) {

	UNUSED(clsSocketUDT);

	const jsize capacity = env->GetArrayLength(arrayObj);

	if (!X_IsValidRange(env, socketID, position, limit, capacity)) {
		return JNI_ERR;
	}

	const jsize size = limit - position;

	jbyte * const data = (jbyte *) malloc(sizeof(jbyte) * size);

	if (data == NULL) {
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"recv/recvmsg : can not allocate data array");
		return JNI_ERR;
	}

	int rv;

	switch (socketType) {
	case SOCK_STREAM:
		rv = UDT::recv(socketID, (char*) data, (int) size, 0);
		break;
	case SOCK_DGRAM:
		rv = UDT::recvmsg(socketID, (char*) data, (int) size);
		break;
	default:
		free(data);
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"recv/recvmsg : unexpected socketType");
		return JNI_ERR;
	}

	if (rv > 0) { // normal
		env->SetByteArrayRegion(arrayObj, position, rv, data);
		free(data);
		return rv;
	} else if (rv < 0) { // UDT::ERROR
		free(data);
		return UDT_ReturnReceiveError(env, socketID);
	} else { // ==0; UDT_TIMEOUT
		free(data);
		return UDT_TIMEOUT;
	}

}

// receive into direct byte buffer
JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_receive2( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint socketID, //
		const jint socketType, //
		const jobject bufferObj, //
		const jint position, //
		const jint limit //
		) {

	UNUSED(clsSocketUDT);

	const jlong capacity = env->GetDirectBufferCapacity(bufferObj);

	if (!X_IsValidRange(env, socketID, position, limit, capacity)) {
		return JNI_ERR;
	}

	const jbyte * bufferAddress = //
			static_cast<jbyte*>(env->GetDirectBufferAddress(bufferObj));

	const jbyte * data = bufferAddress + position;
	const jsize size = static_cast<jsize>(limit - position);

	// memory boundary test
	// assert(data[0] | 1);
	// assert(data[size - 1] | 1);

	int rv;

	switch (socketType) {
	case SOCK_STREAM:
		rv = UDT::recv(socketID, (char*) data, (int) size, 0);
		break;
	case SOCK_DGRAM:
		rv = UDT::recvmsg(socketID, (char*) data, (int) size);
		break;
	default:
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"recv/recvmsg : unexpected socketType");
		return JNI_ERR;
	}

	if (rv > 0) { // normal
		return rv;
	} else if (rv < 0) { //  UDT::ERROR
		return UDT_ReturnReceiveError(env, socketID);
	} else { // ==0; UDT_TIMEOUT
		return UDT_TIMEOUT;
	}

}

/*
 * sendXXX call is shared for both send() and sendmsg()
 */

// return values, if exception is NOT thrown
// -1 : no buffer space (non-blocking only )
// =0 : timeout expired (blocking only)
// >0 : normal send, byte count
jint UDT_ReturnSendError( //
		JNIEnv * const env, //
		const jint socketID //
		) {
	UDT::ERRORINFO errorInfo = UDT::getlasterror();
	const int errorCode = errorInfo.getErrorCode();
	if (errorCode == UDT::ERRORINFO::EASYNCSND) {
		// not a java exception: non-blocking mode return when no space in UDT buffer
	} else {
		// really exception
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "send/sendmsg", &errorInfo);
	}
	return JNI_ERR;
}

// send from complete array
JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_send0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint socketID, //
		const jint socketType, //
		const jint timeToLive, //
		const jboolean isOrdered, //
		const jbyteArray arrayObj //
		) {

	UNUSED(clsSocketUDT);

	jboolean isCopy; // whether JVM returned reference or copy

	jbyte * const data = env->GetByteArrayElements(arrayObj, &isCopy); // note: must release

	const jsize size = env->GetArrayLength(arrayObj);

	int rv;

	switch (socketType) {
	case SOCK_STREAM:
		rv = UDT::send(socketID, (char*) data, (int) size, 0);
		break;
	case SOCK_DGRAM:
		rv = UDT::sendmsg(socketID, (char*) data, (int) size, (int) timeToLive,
				BOOL(isOrdered));
		break;
	default:
		env->ReleaseByteArrayElements(arrayObj, data, JNI_ABORT); // do not copy back
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"send/sendmsg : unexpected socketType");
		return JNI_ERR;
	}

	env->ReleaseByteArrayElements(arrayObj, data, JNI_ABORT); // do not copy back

	if (rv > 0) { // normal
		return rv;
	} else if (rv < 0) { // UDT::ERROR
		return UDT_ReturnSendError(env, socketID);
	} else { // ==0; UDT_TIMEOUT
		return UDT_TIMEOUT;
	}

}

// send from a portion of array
JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_send1( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint socketID, //
		const jint socketType, //
		const jint timeToLive, //
		const jboolean isOrdered, //
		const jbyteArray arrayObj, //
		const jint position, //
		const jint limit //
		) {

	UNUSED(clsSocketUDT);

	const jsize capacity = env->GetArrayLength(arrayObj);

	if (!X_IsValidRange(env, socketID, position, limit, capacity)) {
		return JNI_ERR;
	}

	const jsize size = limit - position;

	jbyte * const data = (jbyte *) malloc(sizeof(jbyte) * size);

	if (data == NULL) {
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"send/sendmsg : can not allocate data array");
		return JNI_ERR;
	}

	env->GetByteArrayRegion(arrayObj, position, size, data);

	int rv;

	switch (socketType) {
	case SOCK_STREAM:
		rv = UDT::send(socketID, (char*) data, (int) size, 0);
		break;
	case SOCK_DGRAM:
		rv = UDT::sendmsg(socketID, (char*) data, (int) size, (int) timeToLive,
				BOOL(isOrdered));
		break;
	default:
		free(data);
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"send/sendmsg : unexpected socketType");
		return JNI_ERR;
	}

	free(data);

	if (rv > 0) { // normal
		return rv;
	} else if (rv < 0) { // UDT::ERROR
		return UDT_ReturnSendError(env, socketID);
	} else { // ==0; UDT_TIMEOUT
		return UDT_TIMEOUT;
	}

}

// send direct byte buffer
JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_send2( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint socketID, //
		const jint socketType, //
		const jint timeToLive, //
		const jboolean isOrdered, //
		const jobject bufferObj, //
		const jint position, //
		const jint limit //
		) {

	UNUSED(clsSocketUDT);

	const jlong capacity = env->GetDirectBufferCapacity(bufferObj);

	if (!X_IsValidRange(env, socketID, position, limit, capacity)) {
		return JNI_ERR;
	}

	jbyte* bufferAddress = static_cast<jbyte*>(env->GetDirectBufferAddress(
			bufferObj));

	const jbyte* data = bufferAddress + position;
	const jsize size = static_cast<jsize>(limit - position);

	// memory boundary test
	// assert(data[0] | 1);
	// assert(data[size - 1] | 1);

	int rv;

	switch (socketType) {
	case SOCK_STREAM:
		rv = UDT::send(socketID, (char*) data, (int) size, 0);
		break;
	case SOCK_DGRAM:
		rv = UDT::sendmsg(socketID, (char*) data, (int) size, (int) timeToLive,
				BOOL(isOrdered));
		break;
	default:
		UDT_ThrowExceptionUDT_Message(env, socketID,
				"send/sendmsg : unexpected socketType");
		return JNI_ERR;
	}

	if (rv > 0) { // normal
		return rv;
	} else if (rv < 0) { // UDT::ERROR
		return UDT_ReturnSendError(env, socketID);
	} else { // ==0; UDT_TIMEOUT
		return UDT_TIMEOUT;
	}

}

JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_getErrorCode0( //
		JNIEnv * const env, //
		const jobject self //
		) {

	UNUSED(env);
	UNUSED(self);

	const jint errorCode = UDT::getlasterror().getErrorCode();

	return errorCode;

}

JNIEXPORT jstring JNICALL Java_com_barchart_udt_SocketUDT_getErrorMessage0( //
		JNIEnv * const env, //
		const jobject self //
		) {

	UNUSED(self);

	const char* errorMessage = UDT::getlasterror().getErrorMessage();

	return (errorMessage == NULL) ?
			env->NewStringUTF("<NONE>") : env->NewStringUTF(errorMessage);

}

JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_clearError0( //
		JNIEnv * const env, const jobject self //
		) {

	UNUSED(env);
	UNUSED(self);

	UDT::getlasterror().clear();

}

JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_updateMonitor0( //
		JNIEnv * const env, //
		const jobject self, //
		const jboolean makeClear //
		) {

	UDT::TRACEINFO monitor;

	const jint socketID = UDT_GetSocketID(env, self);

	const int rv = UDT::perfmon(socketID, &monitor, BOOL(makeClear));

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "updateMonitor0:perfmon", &errorInfo);
		return;
	}

	const jobject objMonitor = env->GetObjectField(self, udts_MonitorID);

	CHK_NUL_RET_RET(objMonitor, "objMonitor");

	// global measurements
	env->SetLongField(objMonitor, udtm_msTimeStamp, monitor.msTimeStamp); // time since the UDT entity is started, in milliseconds
	env->SetLongField(objMonitor, udtm_pktSentTotal, monitor.pktSentTotal); // total number of sent data packets, including retransmissions
	env->SetLongField(objMonitor, udtm_pktRecvTotal, monitor.pktRecvTotal); // total number of received packets
	env->SetIntField(objMonitor, udtm_pktSndLossTotal, monitor.pktSndLossTotal); // total number of lost packets (sender side)
	env->SetIntField(objMonitor, udtm_pktRcvLossTotal, monitor.pktRcvLossTotal); // total number of lost packets (receiver side)
	env->SetIntField(objMonitor, udtm_pktRetransTotal, monitor.pktRetransTotal); // total number of retransmitted packets
	env->SetIntField(objMonitor, udtm_pktSentACKTotal, monitor.pktSentACKTotal); // total number of sent ACK packets
	env->SetIntField(objMonitor, udtm_pktRecvACKTotal, monitor.pktRecvACKTotal); // total number of received ACK packets
	env->SetIntField(objMonitor, udtm_pktSentNAKTotal, monitor.pktSentNAKTotal); // total number of sent NAK packets
	env->SetIntField(objMonitor, udtm_pktRecvNAKTotal, monitor.pktRecvNAKTotal); // total number of received NAK packets
	env->SetLongField(objMonitor, udtm_usSndDurationTotal,
			monitor.usSndDurationTotal); // total time duration when UDT is sending data (idle time exclusive)

	// local measurements
	env->SetLongField(objMonitor, udtm_pktSent, monitor.pktSent); // number of sent data packets, including retransmissions
	env->SetLongField(objMonitor, udtm_pktRecv, monitor.pktRecv); // number of received packets
	env->SetIntField(objMonitor, udtm_pktSndLoss, monitor.pktSndLoss); // number of lost packets (sender side)
	env->SetIntField(objMonitor, udtm_pktRcvLoss, monitor.pktRcvLoss); // number of lost packets (receiverer side)
	env->SetIntField(objMonitor, udtm_pktRetrans, monitor.pktRetrans); // number of retransmitted packets
	env->SetIntField(objMonitor, udtm_pktSentACK, monitor.pktSentACK); // number of sent ACK packets
	env->SetIntField(objMonitor, udtm_pktRecvACK, monitor.pktRecvACK); // number of received ACK packets
	env->SetIntField(objMonitor, udtm_pktSentNAK, monitor.pktSentNAK); // number of sent NAK packets
	env->SetIntField(objMonitor, udtm_pktRecvNAK, monitor.pktRecvNAK); // number of received NAK packets
	env->SetDoubleField(objMonitor, udtm_mbpsSendRate, monitor.mbpsSendRate); // sending rate in Mb/s
	env->SetDoubleField(objMonitor, udtm_mbpsRecvRate, monitor.mbpsRecvRate); // receiving rate in Mb/s
	env->SetLongField(objMonitor, udtm_usSndDuration, monitor.usSndDuration); // busy sending time (i.e., idle time exclusive)

	// instant measurements
	env->SetDoubleField(objMonitor, udtm_usPktSndPeriod,
			monitor.usPktSndPeriod); // packet sending period, in microseconds
	env->SetIntField(objMonitor, udtm_pktFlowWindow, monitor.pktFlowWindow); // flow window size, in number of packets
	env->SetIntField(objMonitor, udtm_pktCongestionWindow,
			monitor.pktCongestionWindow); // congestion window size, in number of packets
	env->SetIntField(objMonitor, udtm_pktFlightSize, monitor.pktFlightSize); // number of packets on flight
	env->SetDoubleField(objMonitor, udtm_msRTT, monitor.msRTT); // RTT, in milliseconds
	env->SetDoubleField(objMonitor, udtm_mbpsBandwidth, monitor.mbpsBandwidth); // estimated bandwidth, in Mb/s
	env->SetIntField(objMonitor, udtm_byteAvailSndBuf, monitor.byteAvailSndBuf); // available UDT sender buffer size
	env->SetIntField(objMonitor, udtm_byteAvailRcvBuf, monitor.byteAvailRcvBuf); // available UDT receiver buffer size

}

// #########################################################################

void UDT_CopyArrayToSet(jint* array, set<UDTSOCKET>* udSet, const jsize size) {
	for (jint index = 0; index < size; index++) {
		const jint socketID = array[index];
		udSet->insert(socketID);
	}
}

void UDT_CopySetToArray(set<UDTSOCKET>* udSet, jint* array, const jsize size) {
	set<UDTSOCKET>::iterator iterator = udSet->begin();
	for (jint index = 0; index < size; index++) {
		const jint socketID = *iterator;
		array[index] = socketID;
		++iterator;
	}
}

#define UDT_READ_INDEX		com_barchart_udt_SocketUDT_UDT_READ_INDEX
#define UDT_WRITE_INDEX		com_barchart_udt_SocketUDT_UDT_WRITE_INDEX
#define UDT_EXCEPT_INDEX	com_barchart_udt_SocketUDT_UDT_EXCEPT_INDEX
#define UDT_SIZE_COUNT		com_barchart_udt_SocketUDT_UDT_SIZE_COUNT
#define UDT_SELECT_LIMIT	com_barchart_udt_SocketUDT_DEFAULT_MAX_SELECTOR_SIZE

JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_getStatus0( //
		JNIEnv * const env, //
		const jobject self //
		) {

	const jint socketID = UDT_GetSocketID(env, self);

	const UDTSTATUS status = UDT::getsockstate(socketID);

	return static_cast<jint>(status);

}

//

JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_epollCreate0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT //
		) {

	UNUSED(env);
	UNUSED(clsSocketUDT);

	const int rv = UDT::epoll_create();

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, 0, "epollCreate0:epoll_create", &errorInfo);
		return JNI_ERR;
	}

	return static_cast<jint>(rv);

}

JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_epollRelease0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint pollID) {

	UNUSED(env);
	UNUSED(clsSocketUDT);

	const int rv = UDT::epoll_release(pollID);

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, 0, "epollRelease0:epoll_release", &errorInfo);
		return;
	}

}

JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_epollAdd0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint pollID, //
		const jint socketID, //
		const jint pollOpt //
		) {

	UNUSED(env);
	UNUSED(clsSocketUDT);

	const int events = static_cast<int>(pollOpt);

	const int rv = UDT::epoll_add_usock(pollID, socketID, &events);

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "epollAdd0:epoll_add_usock", &errorInfo);
		return;
	}

}

JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_epollRemove0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint pollID, //
		const jint socketID //
		) {

	UNUSED(env);
	UNUSED(clsSocketUDT);

	const int rv = UDT::epoll_remove_usock(pollID, socketID);

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "epollRemove0:epoll_remove_usock", &errorInfo);
		return;
	}

}

JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_epollUpdate0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint pollID, //
		const jint socketID, //
		const jint pollOpt //
		) {

	UNUSED(clsSocketUDT);

	const int events = static_cast<int>(pollOpt);

	const int rv = UDT::epoll_update_usock(pollID, socketID, &events);

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "epollUpdate0:epoll_update_usock", &errorInfo);
		return;
	}

}

JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_epollVerify0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint pollID, //
		const jint socketID) {

	UNUSED(clsSocketUDT);

	int events = 0;

	const int rv = UDT::epoll_verify_usock(pollID, socketID, &events);

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "epollVerify0:epoll_update_usock", &errorInfo);
		return JNI_ERR;
	}

	return static_cast<jint>(events);

}

JNIEXPORT jint JNICALL Java_com_barchart_udt_SocketUDT_epollWait0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint pollID, //
		const jobject objReadBuffer, //
		const jobject objWriteBuffer, //
		const jobject objSizeBuffer, //
		const jlong millisTimeout //
		) {

	UNUSED(clsSocketUDT);

	// readiness sets
	set<UDTSOCKET> readSet;
	set<UDTSOCKET> writeSet;

	// readiness report
	const int rv = UDT::epoll_wait( //
			pollID, &readSet, &writeSet, millisTimeout, NULL, NULL);

	// readiness reports size array
	jint* const sizeArray = //
			static_cast<jint*>(env->GetDirectBufferAddress(objSizeBuffer));

	// process timeout & errors
	if (rv <= 0) { // UDT::ERROR is '-1'
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		if (errorInfo.getErrorCode() == UDT::ERRORINFO::ETIMEOUT) {
			// not a java exception:
			sizeArray[UDT_READ_INDEX] = 0;
			sizeArray[UDT_WRITE_INDEX] = 0;
			sizeArray[UDT_EXCEPT_INDEX] = 0;
			return UDT_TIMEOUT;
		} else {
			// really exception
			UDT_ThrowExceptionUDT_ErrorInfo( //
					env, 0, "epollWait0:epoll_wait", &errorInfo);
			return JNI_ERR;
		}
	}

	// return read interest
	const jsize readSize = readSet.size();
	sizeArray[UDT_READ_INDEX] = readSize;
	if (readSize > 0) {
		if (readSize > env->GetDirectBufferCapacity(objReadBuffer)) {
			UDT_ThrowExceptionUDT_Message(env, 0,
					"readSize > objReadBuffer capacity");
			return JNI_ERR;
		}
		jint* const readArray = //
				static_cast<jint*>(env->GetDirectBufferAddress(objReadBuffer));
		UDT_CopySetToArray(&readSet, readArray, readSize);
	}

	// return write interest
	const jsize writeSize = writeSet.size();
	sizeArray[UDT_WRITE_INDEX] = writeSize;
	if (writeSize > 0) {
		if (writeSize > env->GetDirectBufferCapacity(objWriteBuffer)) {
			UDT_ThrowExceptionUDT_Message(env, 0,
					"writeSize > objWriteBuffer capacity");
			return JNI_ERR;
		}
		jint* const writeArray = //
				static_cast<jint*>(env->GetDirectBufferAddress(objWriteBuffer));
		UDT_CopySetToArray(&writeSet, writeArray, writeSize);
	}

	return rv;

}

// #########################################
// #
// # start - used for development only
// #

// test cost of JNI call
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testEmptyCall0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT //
		) {

	UNUSED(env);
	UNUSED(clsSocketUDT);

}

// test cost of JNI-to-Java array iteration
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testIterateArray0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jobjectArray objArray //
		) {

	UNUSED(clsSocketUDT);

	const jsize size = env->GetArrayLength(objArray);
	for (jint index = 0; index < size; index++) {
		jobject objAny = env->GetObjectArrayElement(objArray, index);
		UNUSED(objAny);
		objAny = NULL;
	}

}

// test cost of JNI-to-Java set iteration
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testIterateSet0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jobject objSet //
		) {

	UNUSED(clsSocketUDT);

	jobject iterator = env->CallObjectMethod(objSet, jdk_clsSet_iteratorID);

	jint count = 0;

	while (JNI_TRUE
			== env->CallBooleanMethod(iterator, jdk_clsIterator_hasNextID)) {
		jobject objAny = env->CallObjectMethod( //
				iterator, jdk_clsIterator_nextID);
		UNUSED(objAny);
		objAny = NULL;
		count++;
	}

}

// test cost of JNI-to-Java make array
JNIEXPORT jintArray JNICALL JNICALL Java_com_barchart_udt_SocketUDT_testMakeArray0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint size //
		) {

	UNUSED(clsSocketUDT);

	jintArray array = env->NewIntArray(size);

	return array;

}

// test cost of array copy
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testGetSetArray0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jintArray objArray, //
		const jboolean isReturn //
		) {

	UNUSED(clsSocketUDT);

	jboolean isCopy;

	jint* data = env->GetIntArrayElements(objArray, &isCopy);

	//	jsize size = env->GetArrayLength(objArray);

	if (isReturn == JNI_TRUE) {
		env->ReleaseIntArrayElements(objArray, data, JNI_UPDATE);
	} else {
		env->ReleaseIntArrayElements(objArray, data, JNI_ABORT);
	}

}

// test error on close of a closed socket
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testInvalidClose0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jint socketID //
		) {

	UNUSED(clsSocketUDT);

	const int rv = UDT::close(socketID);

	if (rv == UDT::ERROR) {
		UDT::ERRORINFO errorInfo = UDT::getlasterror();
		UDT_ThrowExceptionUDT_ErrorInfo( //
				env, socketID, "testInvalidClose0:close", &errorInfo);
		return;
	}

}

// test crash jvm to debug crash log parser
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testCrashJVM0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT //
		) {

	printf("native: test crash jvm \n");

	UNUSED(env);
	UNUSED(clsSocketUDT);

	jint *array = NULL;

	array[0] = 1;

}

// test correctness of direct BYTE buffer access
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testDirectByteBufferAccess0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jobject bufferObj //
		) {

	UNUSED(clsSocketUDT);

	jbyte* byteBuffer = //
			static_cast<jbyte*>(env->GetDirectBufferAddress(bufferObj));

	jlong capacity = env->GetDirectBufferCapacity(bufferObj);

	printf("native: byteBuffer capacity=%d \n", (int) capacity);

	byteBuffer[0] = 'A';
	byteBuffer[1] = 'B';
	byteBuffer[2] = 'C';
	byteBuffer[3] = 'D';
	byteBuffer[4] = 'E';
	byteBuffer[5] = 'F';
	byteBuffer[6] = 'G';
	byteBuffer[7] = 'H';

}

// test correctness of direct INT buffer access
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testDirectIntBufferAccess0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jobject bufferObj //
		) {

	UNUSED(clsSocketUDT);

	jint* intBuffer = static_cast<jint*>(env->GetDirectBufferAddress(bufferObj));

	jlong capacity = env->GetDirectBufferCapacity(bufferObj);

	printf("native: intBuffer capacity=%d \n", (int) capacity);

	intBuffer[0] = 'A';
	intBuffer[1] = 'B';
	intBuffer[2] = 'C';
	intBuffer[3] = 'D';
	intBuffer[4] = 'E';
	intBuffer[5] = 'F';
	intBuffer[6] = 'G';
	intBuffer[7] = 'H';

}

// test cost of jni array fill
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testFillArray0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jbyteArray arrayObj) {

	UNUSED(clsSocketUDT);

	jboolean isCopy;
	jbyte* array = env->GetByteArrayElements(arrayObj, &isCopy);
	jsize size = env->GetArrayLength(arrayObj);

	for (int k = 0; k < size; k++) {
		array[k] = (char) k;
	}

	env->ReleaseByteArrayElements(arrayObj, array, JNI_UPDATE);

}

// test cost of jni direct buffer fill
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testFillBuffer0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jobject bufferObj //
		) {

	UNUSED(clsSocketUDT);

	jbyte* buffer = static_cast<jbyte*>(env->GetDirectBufferAddress(bufferObj));

	jlong capacity = env->GetDirectBufferCapacity(bufferObj);

	for (int k = 0; k < capacity; k++) {
		buffer[k] = (char) k;
	}

}

// test cost of direct INT buffer load
JNIEXPORT void JNICALL Java_com_barchart_udt_SocketUDT_testDirectIntBufferLoad0( //
		JNIEnv * const env, //
		const jclass clsSocketUDT, //
		const jobject bufferObj //
		) {

	UNUSED(clsSocketUDT);

	const jint* byteBuffer = //
			static_cast<jint*>(env->GetDirectBufferAddress(bufferObj));

	UNUSED(byteBuffer);

	const jlong capacity = env->GetDirectBufferCapacity(bufferObj);

	UNUSED(capacity);

}

// #
// # finish - used for development only
// #
// #########################################

}// [extern "C"]
