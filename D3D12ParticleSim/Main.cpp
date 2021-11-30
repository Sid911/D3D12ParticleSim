// D3D12ParticleSim.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "D3D12ParticleSim.h"
#include "pch.h"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	D3D12ParticleSim win(1280, 720, L"Particle Stuff");
	return Win32Application::Run(&win, hInstance, nCmdShow);
}