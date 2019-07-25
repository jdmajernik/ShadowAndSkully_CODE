// Fill out your copyright notice in the Description page of Project Settings.


#include "MazeNode.h"

void AMazeNode::ActivateLever_Implementation(bool NewState)
{
	State = NewState;
}
void AMazeNode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AMazeNode::BeginPlay()
{
	Super::BeginPlay();
	DefaultState = GetActorRotation();
}
