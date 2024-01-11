#pragma once

#include <exception>

class BadWeakPtr : public std::exception {};

template <typename T>
class SharedPtr;

template <typename T>
class WeakPtr;

class ControlBlockBase;

template <typename T>
class PtrControlBlock;

template <typename T>
class ObjectControlBlock;
