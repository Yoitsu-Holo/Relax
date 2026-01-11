package main

/*
#cgo CFLAGS: -I../../../cache-Interface
#cgo LDFLAGS: -L../../../build/cache-Interface -lcache_interface -lstdc++
#include "type_driver_c.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
*/
import "C"
import (
	"fmt"
	"unsafe"
)

func main() {
	fmt.Println("=== Testing C Interface Directly ===")

	// Create handle
	handle := C.type_driver_create()
	if handle == nil {
		fmt.Println("FAIL: Failed to create handle")
		return
	}
	fmt.Println("OK: Handle created")

	// Initialize
	ret := C.type_driver_init(handle, 16)
	fmt.Printf("Init result: %d (0 means OK)\n", ret)
	if ret != 0 {
		fmt.Printf("FAIL: Initialization failed with code %d\n", ret)
		C.type_driver_destroy(handle)
		return
	}
	fmt.Println("OK: Initialized")

	// Check if initialized
	isInit := C.type_driver_is_initialized(handle)
	fmt.Printf("Is initialized: %d\n", isInit)

	// Test KV Set
	value := "test_value"
	cValue := C.CString(value)
	defer C.free(unsafe.Pointer(cValue))

	ret = C.type_driver_kv_set(handle, 12345, 67890, cValue, C.size_t(len(value)))
	fmt.Printf("KV Set result: %d\n", ret)
	if ret != 0 {
		fmt.Printf("FAIL: KV Set failed with code %d\n", ret)
	} else {
		fmt.Println("OK: KV Set succeeded")
	}

	// Test KV Get
	var retData *C.char
	var retLen C.size_t
	ret = C.type_driver_kv_get(handle, 12345, 67890, &retData, &retLen)
	fmt.Printf("KV Get result: %d\n", ret)
	if ret != 0 {
		fmt.Printf("FAIL: KV Get failed with code %d\n", ret)
	} else if retData != nil {
		getValue := C.GoStringN(retData, C.int(retLen))
		fmt.Printf("OK: Got value: %s\n", getValue)
	} else {
		fmt.Println("WARN: retData is nil")
	}

	// Test KV Exists
	ret = C.type_driver_kv_exists(handle, 12345, 67890)
	fmt.Printf("KV Exists result: %d (1 means exists)\n", ret)

	// Clean up
	C.type_driver_destroy(handle)
	fmt.Println("OK: Handle destroyed")
}
