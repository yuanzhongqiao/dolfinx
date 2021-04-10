// Copyright (C) 2021 Garth N. Wells, Jørgen S. Dokken, Igor A. Baratta
//
// This file is part of DOLFINX (https://www.fenicsproject.org)
//
// SPDX-License-Identifier:    LGPL-3.0-or-later

#include "interpolate.h"
#include "FiniteElement.h"
#include <dolfinx/mesh/Mesh.h>
#include <xtensor/xtensor.hpp>
#include <xtensor/xview.hpp>

using namespace dolfinx;

//-----------------------------------------------------------------------------
xt::xtensor<double, 2>
fem::interpolation_coords(const fem::FiniteElement& element,
                          const mesh::Mesh& mesh,
                          const tcb::span<const std::int32_t>& cells)
{
  // Get mesh geometry data and the element coordinate map
  const int gdim = mesh.geometry().dim();
  const graph::AdjacencyList<std::int32_t>& x_dofmap = mesh.geometry().dofmap();
  const int num_dofs_g = x_dofmap.num_links(0);
  const array2d<double>& x_g = mesh.geometry().x();

  const fem::CoordinateElement& cmap = mesh.geometry().cmap();

  // Get the interpolation points on the reference cells
  const array2d<double> X = element.interpolation_points();

  const xt::xtensor<double, 2> phi
      = xt::view(cmap.tabulate(0, X), 0, xt::all(), xt::all(), 0);

  // Push reference coordinates (X) forward to the physical coordinates
  // (x) for each cell
  array2d<double> x_cell(X.shape[0], gdim);
  array2d<double> coordinate_dofs(num_dofs_g, gdim);
  std::array<std::size_t, 2> shape = {3, cells.size() * X.shape[0]};
  xt::xtensor<double, 2> x = xt::zeros<double>(shape);
  for (std::size_t c = 0; c < cells.size(); ++c)
  {
    // Get geometry data for current cell
    auto x_dofs = x_dofmap.links(cells[c]);
    for (int i = 0; i < num_dofs_g; ++i)
      for (int j = 0; j < gdim; ++j)
        coordinate_dofs(i, j) = x_g(x_dofs[i], j);

    // Push forward coordinates (X -> x)
    cmap.push_forward(x_cell, coordinate_dofs, phi);
    for (std::size_t i = 0; i < x_cell.shape[0]; ++i)
      for (std::size_t j = 0; j < x_cell.shape[1]; ++j)
        x(j, c * X.shape[0] + i) = x_cell(i, j);
  }

  return x;
}
//-----------------------------------------------------------------------------