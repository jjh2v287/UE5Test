// Fill out your copyright notice in the Description page of Project Settings.


#include "DEPRECATED_Mutex.h"

void UDEPRECATED_Mutex::BeginDestroy()
{
	UObject::BeginDestroy();
	Mutex.Unlock();
}
