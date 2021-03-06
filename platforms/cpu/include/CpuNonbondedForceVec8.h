
/* Portions copyright (c) 2006-2015 Stanford University and Simbios.
 * Contributors: Pande Group
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef OPENMM_CPU_NONBONDED_FORCE_VEC8_H__
#define OPENMM_CPU_NONBONDED_FORCE_VEC8_H__

#include "CpuNonbondedForce.h"

#ifdef __AVX__

#include "openmm/internal/vectorize8.h"

// ---------------------------------------------------------------------------------------

namespace OpenMM {

class CpuNonbondedForceVec8 : public CpuNonbondedForce {
public:
       CpuNonbondedForceVec8();

protected:            
      /**---------------------------------------------------------------------------------------
      
         Calculate all the interactions for one atom block. These are part of the virtual function interface.
         They internally call into  the generic handler function below.
      
         @param blockIndex       the index of the atom block
         @param forces           force array (forces added)
         @param totalEnergy      total energy
            
         --------------------------------------------------------------------------------------- */
      ///@{    
      void calculateBlockIxn(int blockIndex, float* forces, double* totalEnergy, const fvec4& boxSize, const fvec4& invBoxSize);
      void calculateBlockEwaldIxn(int blockIndex, float* forces, double* totalEnergy, const fvec4& boxSize, const fvec4& invBoxSize);
      ///@}

      /**---------------------------------------------------------------------------------------
      
         Calculate all the interactions for one atom block. Identical to function prototypes above but
         with an extra template parameter to choose whether to use Ewald processing or not.
         --------------------------------------------------------------------------------------- */
      template<bool IS_EWALD>
      void calculateBlockIxnHandler(int blockIndex, float* forces, double* totalEnergy, const fvec4& boxSize, const fvec4& invBoxSize);
      
      /**
       * Templatized implementation of calculateBlockIxn. It can handle both Ewald and non-ewald interactions
       * through a template parameter since the code is so similar for the two cases.
       */
      template <int PERIODIC_TYPE, bool IS_EWALD>
      void calculateBlockIxnImpl(int blockIndex, float* forces, double* totalEnergy, const fvec4& boxSize, const fvec4& invBoxSize, const fvec4& blockCenter);

      /**
       * Compute the displacement and squared distance between a collection of points, optionally using
       * periodic boundary conditions.
       */
      template <int PERIODIC_TYPE>
      void getDeltaR(const fvec4& posI, const fvec8& x, const fvec8& y, const fvec8& z, fvec8& dx, fvec8& dy, fvec8& dz, fvec8& r2, bool periodic, const fvec4& boxSize, const fvec4& invBoxSize) const;

      /// Compute an approximation of a function using a table lookup.
      fvec8 approximateFunctionFromTable(const std::vector<float>& table, fvec8 x, fvec8 inverse) const;

};

} // namespace OpenMM

// ---------------------------------------------------------------------------------------

#endif // __AVX__

#endif // OPENMM_CPU_NONBONDED_FORCE_VEC8_H__
