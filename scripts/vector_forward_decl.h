#pragma once

// NOTE(Mara): need to use this to forward declare to get the default growth step parameter without compiler warnings/errors
template <typename T, int VEC_GROWTH_STEP = 0> class VectorClass;
template <typename T, int VEC_GROWTH_STEP = 0> class DynamicVectorClass;
template <typename T, int VEC_GROWTH_STEP = 0> class SimpleVecClass;
template <typename T, int VEC_GROWTH_STEP = 0> class SimpleDynVecClass;