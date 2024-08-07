/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     | Version:     3.2
    \\  /    A nd           | Web:         http://www.foam-extend.org
     \\/     M anipulation  | For copyright notice see file Copyright
-------------------------------------------------------------------------------
License
    This file is part of foam-extend.

    foam-extend is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    foam-extend is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with foam-extend.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "sequential.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class CompType, class ThermoType>
Foam::sequential<CompType, ThermoType>::sequential
(
    ODEChemistryModel<CompType, ThermoType>& model,
    const word& modelName
)
:
    chemistrySolver<CompType, ThermoType>(model, modelName),
    coeffsDict_(model.subDict(modelName + "Coeffs")),
    cTauChem_(readScalar(coeffsDict_.lookup("cTauChem"))),
    equil_(coeffsDict_.lookup("equilibriumRateLimiter"))
{}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

template<class CompType, class ThermoType>
Foam::sequential<CompType, ThermoType>::~sequential()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class CompType, class ThermoType>
Foam::scalar Foam::sequential<CompType, ThermoType>::solve
(
    scalarField &c,
    const scalar T,
    const scalar p,
    const scalar t0,
    const scalar dt
) const
{
    scalar tChemInv = SMALL;

    scalar pf, cf, pb, cb;
    label lRef, rRef;

    for (label i=0; i<this->model_.reactions().size(); i++)
    {
        const Reaction<ThermoType>& R = this->model_.reactions()[i];

        scalar om0 = this->model_.omega
        (
            R, c, T, p, pf, cf, lRef, pb, cb, rRef
        );

        scalar omeg = 0.0;
        if (!equil_)
        {
            omeg = om0;
        }
        else
        {
            if (om0<0.0)
            {
                omeg = om0/(1.0 + pb*dt);
            }
            else
            {
                omeg = om0/(1.0 + pf*dt);
            }
        }
        tChemInv = max(tChemInv, mag(omeg));


        // update species
        for (label s=0; s<R.lhs().size(); s++)
        {
            label si = R.lhs()[s].index;
            scalar sl = R.lhs()[s].stoichCoeff;
            c[si] -= dt*sl*omeg;
            c[si] = max(0.0, c[si]);
        }

        for (label s=0; s<R.rhs().size(); s++)
        {
            label si = R.rhs()[s].index;
            scalar sr = R.rhs()[s].stoichCoeff;
            c[si] += dt*sr*omeg;
            c[si] = max(0.0, c[si]);
        }

    } // end for (label i...

    return cTauChem_/tChemInv;
}


// ************************************************************************* //
