#pragma once

#ifndef STACK_ALLOC_H
#define STACK_ALLOC_H

#ifdef USE_ALLOCA
#ifdef WIN32
#include <malloc.h>
#else
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else
#include <stdlib.h>
#endif
#endif
#endif

/**
 * @def ALIGN(stack, size)
 *
 * Aligns the stack to a 'size' boundary
 *
 * @param stack Stack
 * @param size  New size boundary
 */

/**
 * @def PUSH(stack, size, type)
 *
 * Allocates 'size' elements of type 'type' on the stack
 *
 * @param stack Stack
 * @param size  Number of elements
 * @param type  Type of element
 */

/**
 * @def VARDECL(var)
 *
 * Declare variable on stack
 *
 * @param var Variable to declare
 */

/**
 * @def ALLOC(var, size, type)
 *
 * Allocate 'size' elements of 'type' on stack
 *
 * @param var  Name of variable to allocate
 * @param size Number of elements
 * @param type Type of element
 */

#ifdef ENABLE_VALGRIND

#include <valgrind/memcheck.h>

#define ALIGN(stack, size) ((stack) += ((size) - (long)(stack)) & ((size) - 1))

#define PUSH(stack, size, type)                                                                                        \
    (VALGRIND_MAKE_NOACCESS(stack, 1000), ALIGN((stack), sizeof(type)),                                                \
     VALGRIND_MAKE_WRITABLE(stack, ((size) * sizeof(type))), (stack) += ((size) * sizeof(type)),                       \
     (type *)((stack) - ((size) * sizeof(type))))

#else

#define ALIGN(stack, size) ((stack) += ((size) - (long)(stack)) & ((size) - 1))

#define PUSH(stack, size, type)                                                                                        \
    (ALIGN((stack), sizeof(type)), (stack) += ((size) * sizeof(type)), (type *)((stack) - ((size) * sizeof(type))))

#endif

#if defined(VAR_ARRAYS)
#define VARDECL(var)
#define ALLOC(var, size, type) type var[size]
#elif defined(USE_ALLOCA)
#define VARDECL(var) var
#define ALLOC(var, size, type) var = alloca(sizeof(type) * (size))
#else
#define VARDECL(var) var
#define ALLOC(var, size, type) var = PUSH(stack, size, type)
#endif

#endif
