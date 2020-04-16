
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

#include "SimTKOpenMMUtilities.h"
#include "CpuNonbondedForceVec8.h"

#include <algorithm>

using namespace std;
using namespace OpenMM;

#ifdef _MSC_VER
    // Workaround for a compiler bug in Visual Studio 10. Hopefully we can remove this
    // once we move to a later version.
    #undef __AVX__
#endif

#ifndef __AVX__
bool isVec8Supported() {
    return false;
}

CpuNonbondedForce* createCpuNonbondedForceVec8() {
    throw OpenMMException("Internal error: OpenMM was compiled without AVX support");
}
#else
/**
 * Check whether 8 component vectors are supported with the current CPU.
 */
bool isVec8Supported() {
    // Make sure the CPU supports AVX.

    int cpuInfo[4];
    cpuid(cpuInfo, 0);
    if (cpuInfo[0] >= 1) {
        cpuid(cpuInfo, 1);
        return ((cpuInfo[2] & ((int) 1 << 28)) != 0);
    }
    return false;
}

/**
 * Factory method to create a CpuNonbondedForceVec8.
 */
CpuNonbondedForce* createCpuNonbondedForceVec8() {
    return new CpuNonbondedForceVec8();
}

/// Use a table lookup to approximate a function specific function.
fvec8
CpuNonbondedForceVec8::approximateFunctionFromTable(const std::vector<float>& table,
                                                    fvec8 x, fvec8 inverse) const
{
    // Compute the set of 8 index positions from which to gather the table data.
    const fvec8 x1 = x * inverse;
    const ivec8 index = min(floor(x1), float(NUM_TABLE_POINTS));

    fvec8 s1, s2;
    gatherVecPair(table.data(), index, s1, s2);

    fvec8 coeff2 = x1-fvec8(index);
    fvec8 coeff1 = 1.0f-coeff2;

    return coeff1*s1 + coeff2*s2;
}

/**---------------------------------------------------------------------------------------

   CpuNonbondedForceVec8 constructor

   --------------------------------------------------------------------------------------- */

CpuNonbondedForceVec8::CpuNonbondedForceVec8() {
}

enum PeriodicType {NoPeriodic, PeriodicPerAtom, PeriodicPerInteraction, PeriodicTriclinic};

void CpuNonbondedForceVec8::calculateBlockIxn(int blockIndex, float* forces, double* totalEnergy, const fvec4& boxSize, const fvec4& invBoxSize)
{
    calculateBlockIxnHandler<false>(blockIndex, forces, totalEnergy, boxSize, invBoxSize);
}

void CpuNonbondedForceVec8::calculateBlockEwaldIxn(int blockIndex, float* forces, double* totalEnergy, const fvec4& boxSize, const fvec4& invBoxSize)
{
    calculateBlockIxnHandler<true>(blockIndex, forces, totalEnergy, boxSize, invBoxSize);
}

template<bool IS_EWALD>
void CpuNonbondedForceVec8::calculateBlockIxnHandler(int blockIndex, float* forces, double* totalEnergy, const fvec4& boxSize, const fvec4& invBoxSize)
{
    // Determine whether we need to apply periodic boundary conditions.
    
    PeriodicType periodicType;
    fvec4 blockCenter;
    if (!periodic) {
        periodicType = NoPeriodic;
        blockCenter = 0.0f;
    }
    else {
        const int* blockAtom = &neighborList->getSortedAtoms()[8*blockIndex];
        float minx, maxx, miny, maxy, minz, maxz;
        minx = maxx = posq[4*blockAtom[0]];
        miny = maxy = posq[4*blockAtom[0]+1];
        minz = maxz = posq[4*blockAtom[0]+2];
        for (int i = 1; i < 8; i++) {
            minx = min(minx, posq[4*blockAtom[i]]);
            maxx = max(maxx, posq[4*blockAtom[i]]);
            miny = min(miny, posq[4*blockAtom[i]+1]);
            maxy = max(maxy, posq[4*blockAtom[i]+1]);
            minz = min(minz, posq[4*blockAtom[i]+2]);
            maxz = max(maxz, posq[4*blockAtom[i]+2]);
        }
        blockCenter = fvec4(0.5f*(minx+maxx), 0.5f*(miny+maxy), 0.5f*(minz+maxz), 0.0f);
        if (!(minx < cutoffDistance || miny < cutoffDistance || minz < cutoffDistance ||
                maxx > boxSize[0]-cutoffDistance || maxy > boxSize[1]-cutoffDistance || maxz > boxSize[2]-cutoffDistance))
            periodicType = NoPeriodic;
        else if (triclinic)
            periodicType = PeriodicTriclinic;
        else if (0.5f*(boxSize[0]-(maxx-minx)) >= cutoffDistance &&
                 0.5f*(boxSize[1]-(maxy-miny)) >= cutoffDistance &&
                 0.5f*(boxSize[2]-(maxz-minz)) >= cutoffDistance)
            periodicType = PeriodicPerAtom;
        else
            periodicType = PeriodicPerInteraction;
    }
    
    // Call the appropriate version depending on what calculation is required for periodic boundary conditions.
    
    if (periodicType == NoPeriodic)
        calculateBlockIxnImpl<NoPeriodic, IS_EWALD>(blockIndex, forces, totalEnergy, boxSize, invBoxSize, blockCenter);
    else if (periodicType == PeriodicPerAtom)
        calculateBlockIxnImpl<PeriodicPerAtom, IS_EWALD>(blockIndex, forces, totalEnergy, boxSize, invBoxSize, blockCenter);
    else if (periodicType == PeriodicPerInteraction)
        calculateBlockIxnImpl<PeriodicPerInteraction, IS_EWALD>(blockIndex, forces, totalEnergy, boxSize, invBoxSize, blockCenter);
    else if (periodicType == PeriodicTriclinic)
        calculateBlockIxnImpl<PeriodicTriclinic, IS_EWALD>(blockIndex, forces, totalEnergy, boxSize, invBoxSize, blockCenter);
}

template <int PERIODIC_TYPE, bool IS_EWALD>
void CpuNonbondedForceVec8::calculateBlockIxnImpl(int blockIndex, float* forces, double* totalEnergy, const fvec4& boxSize, const fvec4& invBoxSize, const fvec4& blockCenter) {
    // Load the positions and parameters of the atoms in the block.
    
    const int* blockAtom = &neighborList->getSortedAtoms()[8*blockIndex];
    fvec4 blockAtomPosq[8];
    fvec8 blockAtomForceX(0.0f), blockAtomForceY(0.0f), blockAtomForceZ(0.0f);
    fvec8 blockAtomX, blockAtomY, blockAtomZ, blockAtomCharge;
    for (int i = 0; i < 8; i++) {
        blockAtomPosq[i] = fvec4(posq+4*blockAtom[i]);
        if (PERIODIC_TYPE == PeriodicPerAtom)
            blockAtomPosq[i] -= floor((blockAtomPosq[i]-blockCenter)*invBoxSize+0.5f)*boxSize;
    }
    transpose(blockAtomPosq[0], blockAtomPosq[1], blockAtomPosq[2], blockAtomPosq[3], blockAtomPosq[4], blockAtomPosq[5], blockAtomPosq[6], blockAtomPosq[7], blockAtomX, blockAtomY, blockAtomZ, blockAtomCharge);
    blockAtomCharge *= ONE_4PI_EPS0;
    // :TODO: Use gather. Use pair-wise gather perhaps?
    fvec8 blockAtomSigma(atomParameters[blockAtom[0]].first, atomParameters[blockAtom[1]].first, atomParameters[blockAtom[2]].first, atomParameters[blockAtom[3]].first, atomParameters[blockAtom[4]].first, atomParameters[blockAtom[5]].first, atomParameters[blockAtom[6]].first, atomParameters[blockAtom[7]].first);
    fvec8 blockAtomEpsilon(atomParameters[blockAtom[0]].second, atomParameters[blockAtom[1]].second, atomParameters[blockAtom[2]].second, atomParameters[blockAtom[3]].second, atomParameters[blockAtom[4]].second, atomParameters[blockAtom[5]].second, atomParameters[blockAtom[6]].second, atomParameters[blockAtom[7]].second);
    const bool needPeriodic = (PERIODIC_TYPE == PeriodicPerInteraction || PERIODIC_TYPE == PeriodicTriclinic);
    const float invSwitchingInterval = 1/(cutoffDistance-switchingDistance);
    const fvec8 cutoffDistanceSquared = cutoffDistance * cutoffDistance;

    // :TODO: Use gather utility.
    const fvec8 C6s =
      IS_EWALD
      ? (C6params[blockAtom[0]], C6params[blockAtom[1]], C6params[blockAtom[2]], C6params[blockAtom[3]], C6params[blockAtom[4]], C6params[blockAtom[5]], C6params[blockAtom[6]], C6params[blockAtom[7]])
      : fvec8();

    // Loop over neighbors for this block.
    
    const vector<int>& neighbors = neighborList->getBlockNeighbors(blockIndex);
    const vector<char>& exclusions = neighborList->getBlockExclusions(blockIndex);
    for (int i = 0; i < (int) neighbors.size(); i++) {
        // Load the next neighbor.
        
        int atom = neighbors[i];
        
        // Compute the distances to the block atoms.
        
        fvec8 dx, dy, dz, r2;
        fvec4 atomPos(posq+4*atom);
        if (PERIODIC_TYPE == PeriodicPerAtom)
            atomPos -= floor((atomPos-blockCenter)*invBoxSize+0.5f)*boxSize;
        getDeltaR<PERIODIC_TYPE>(atomPos, blockAtomX, blockAtomY, blockAtomZ, dx, dy, dz, r2, needPeriodic, boxSize, invBoxSize);

        const int8_t include = ~exclusions[i] & getMaskFromCompare(r2 < cutoffDistanceSquared);
        if (include == 0)
            continue;

        // Compute the interactions.
        
        fvec8 inverseR = rsqrt(r2);
        fvec8 r = r2*inverseR;
        fvec8 energy, dEdR;
        float atomEpsilon = atomParameters[atom].second;
        if (atomEpsilon != 0.0f) {
            fvec8 sig = blockAtomSigma+atomParameters[atom].first;
            fvec8 sig2 = inverseR*sig;
            sig2 *= sig2;
            fvec8 sig6 = sig2*sig2*sig2;
            fvec8 eps = blockAtomEpsilon*atomEpsilon;
            fvec8 epsSig6 = eps*sig6;
            dEdR = epsSig6*(12.0f*sig6 - 6.0f);
            energy = epsSig6*(sig6-1.0f);
            if (useSwitch) {
                fvec8 t = (r>switchingDistance) & ((r-switchingDistance)*invSwitchingInterval);
                fvec8 switchValue = 1+t*t*t*(-10.0f+t*(15.0f-t*6.0f));
                fvec8 switchDeriv = t*t*(-30.0f+t*(60.0f-t*30.0f))*invSwitchingInterval;
                dEdR = switchValue*dEdR - energy*switchDeriv*r;
                energy *= switchValue;
            }
            if (IS_EWALD && ljpme) {
                fvec8 C6ij = C6s*C6params[atom];
                fvec8 inverseR2 = inverseR*inverseR;
                fvec8 mysig2 = sig*sig;
                fvec8 mysig6 = mysig2*mysig2*mysig2;
                fvec8 emult = C6ij*inverseR2*inverseR2*inverseR2*approximateFunctionFromTable(exptermsTable, r, exptermsDXInv);
                fvec8 potentialShift = eps*(1.0f-mysig6*inverseRcut6)*mysig6*inverseRcut6 - C6ij*inverseRcut6Expterm;
                dEdR += 6.0f*C6ij*inverseR2*inverseR2*inverseR2*approximateFunctionFromTable(dExptermsTable, r, exptermsDXInv);
                energy += emult + potentialShift;
            }
        }
        else {
            energy = 0.0f;
            dEdR = 0.0f;
        }
        fvec8 chargeProd = blockAtomCharge*posq[4*atom+3];
        if (IS_EWALD)
            dEdR += chargeProd*inverseR*approximateFunctionFromTable(ewaldScaleTable, r, ewaldDXInv);
        else
        {
            if (cutoff)
                dEdR += chargeProd*(inverseR-2.0f*krf*r2);
            else
                dEdR += chargeProd*inverseR;
        }
        dEdR *= inverseR*inverseR;

        // Accumulate energies.

        fvec8 one(1.0f);
        if (totalEnergy) {
            if (IS_EWALD)
            {
                energy += chargeProd*inverseR*approximateFunctionFromTable(erfcTable, alphaEwald*r, erfcDXInv);
            }
            else
            {
                if (cutoff)
                    energy += chargeProd*(inverseR+krf*r2-crf);
                else
                    energy += chargeProd*inverseR;
            }
            energy = blend(0.0f, energy, include);
            *totalEnergy += dot8(energy, one);
        }

        // Accumulate forces.

        dEdR = blend(0.0f, dEdR, include);
        fvec8 fx = dx*dEdR;
        fvec8 fy = dy*dEdR;
        fvec8 fz = dz*dEdR;
        blockAtomForceX += fx;
        blockAtomForceY += fy;
        blockAtomForceZ += fz;

        float* atomForce = forces+4*atom;
        const fvec4 newAtomForce = fvec4(atomForce) - reduceToVec3(fx, fy, fz);
        _mm_maskstore_ps(atomForce, _mm_setr_epi32(-1, -1, -1, 0), newAtomForce);
    }
    
    // Record the forces on the block atoms.
    
    fvec4 f[8];
    transpose(blockAtomForceX, blockAtomForceY, blockAtomForceZ, 0.0f, f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]);
    for (int j = 0; j < 8; j++)
        (fvec4(forces+4*blockAtom[j])+f[j]).store(forces+4*blockAtom[j]);
}

template <int PERIODIC_TYPE>
void CpuNonbondedForceVec8::getDeltaR(const fvec4& posI, const fvec8& x, const fvec8& y, const fvec8& z, fvec8& dx, fvec8& dy, fvec8& dz, fvec8& r2, bool periodic, const fvec4& boxSize, const fvec4& invBoxSize) const {
    dx = x-posI[0];
    dy = y-posI[1];
    dz = z-posI[2];
    if (PERIODIC_TYPE == PeriodicTriclinic) {
        fvec8 scale3 = floor(dz*recipBoxSize[2]+0.5f);
        dx -= scale3*periodicBoxVectors[2][0];
        dy -= scale3*periodicBoxVectors[2][1];
        dz -= scale3*periodicBoxVectors[2][2];
        fvec8 scale2 = floor(dy*recipBoxSize[1]+0.5f);
        dx -= scale2*periodicBoxVectors[1][0];
        dy -= scale2*periodicBoxVectors[1][1];
        fvec8 scale1 = floor(dx*recipBoxSize[0]+0.5f);
        dx -= scale1*periodicBoxVectors[0][0];
    }
    else if (PERIODIC_TYPE == PeriodicPerInteraction) {
        dx -= round(dx*invBoxSize[0])*boxSize[0];
        dy -= round(dy*invBoxSize[1])*boxSize[1];
        dz -= round(dz*invBoxSize[2])*boxSize[2];
    }
    r2 = dx*dx + dy*dy + dz*dz;
}

#endif
