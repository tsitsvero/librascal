/**
 * file   adaptor_increase_maxorder.hh
 *
 * @author Markus Stricker <markus.stricker@epfl.ch>
 *
 * @date   19 Jun 2018
 *
 * @brief implements an adaptor for structure_managers, which
 * creates a full and half neighbourlist if there is none and
 * triplets/quadruplets, etc. if existent.
 *
 * Copyright © 2018 Markus Stricker, COSMO (EPFL), LAMMM (EPFL)
 *
 * librascal is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3, or (at
 * your option) any later version.
 *
 * librascal is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Emacs; see the file COPYING. If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "structure_managers/structure_manager.hh"
#include "structure_managers/property.hh"
#include "rascal_utility.hh"
#include "lattice.hh"
#include "basic_types.hh"

#include <typeinfo>
#include <set>
#include <vector>

#ifndef ADAPTOR_MAXORDER_H
#define ADAPTOR_MAXORDER_H

namespace rascal {
  /**
   * Forward declaration for traits
   */
  template <class ManagerImplementation>
  class AdaptorMaxOrder;

  /**
   * Specialisation of traits for increase <code>MaxOrder</code> adaptor
   */
  template <class ManagerImplementation>
  struct StructureManager_traits<AdaptorMaxOrder<ManagerImplementation>> {

    constexpr static AdaptorTraits::Strict Strict{AdaptorTraits::Strict::no};
    constexpr static bool HasDistances{false};
    constexpr static bool HasDirectionVectors{
      ManagerImplementation::traits::HasDirectionVectors};
    constexpr static int Dim{ManagerImplementation::traits::Dim};
    //! New MaxOrder upon construction!
    constexpr static size_t MaxOrder{ManagerImplementation::traits::MaxOrder+1};
    //! New Layer
    //! TODO: Is this the correct way to initialize the increased order?
    using LayerByOrder =
      typename LayerExtender<MaxOrder,
                             typename
                             ManagerImplementation::traits::LayerByOrder>::type;
  };

  /**
   * Adaptor that increases the MaxOrder of an existing
   * StructureManager. This means, if the manager does not have a
   * neighbourlist, it is created, if it exists, triplets, quadruplets,
   * etc. lists are created.
   */
  template <class ManagerImplementation>
  class AdaptorMaxOrder: public
  StructureManager<AdaptorMaxOrder<ManagerImplementation>>
  {
  public:
    using Base = StructureManager<AdaptorMaxOrder<ManagerImplementation>>;

    using Parent =
      StructureManager<AdaptorMaxOrder<ManagerImplementation>>;
    using traits = StructureManager_traits<AdaptorMaxOrder>;
    using AtomRef_t = typename ManagerImplementation::AtomRef_t;
    template<size_t Order>
    using ClusterRef_t =
      typename ManagerImplementation::template ClusterRef<Order>;
    //! template<size_t Order, size_t Layer>
    //! using ClusterRefKey_t =
    //!   typename ManagerImplementation::template ClusterRefKey<Order, Layer>;
    //using PairRef_t = ClusterRef_t<2, traits::MaxOrder>;

    static_assert(traits::MaxOrder > 1,
                  "ManagerImplementation needs an atom list.");

    //! Default constructor
    AdaptorMaxOrder() = delete;

    /**
     * Constructs a strict neighbourhood list from a given manager and cut-off
     * radius
     */
    AdaptorMaxOrder(ManagerImplementation& manager, double cutoff);

    //! Copy constructor
    AdaptorMaxOrder(const AdaptorMaxOrder &other) = delete;

    //! Move constructor
    AdaptorMaxOrder(AdaptorMaxOrder &&other) = default;

    //! Destructor
    virtual ~AdaptorMaxOrder() = default;

    //! Copy assignment operator
    AdaptorMaxOrder& operator=(const AdaptorMaxOrder &other) = delete;

    //! Move assignment operator
    AdaptorMaxOrder& operator=(AdaptorMaxOrder &&other) = default;

    //! Updates just the adaptor assuming the underlying manager was
    //! updated. this function invokes building either the neighbour list or to
    //! make triplets, quadruplets, etc. depending on the MaxOrder
    void update();

    // template <size_t MaxOrder, typename std::enable_if<MaxOrder == 2>::value>
    // void update();

    //! Updates the underlying manager as well as the adaptor
    template<class ... Args>
    void update(Args&&... arguments);

    //! Returns cutoff radius of the neighbourhood manager
    inline double get_cutoff() const {return this->cutoff;}

    /** Returns the linear indices of the clusters (whose atom indices
     * are stored in counters). For example when counters is just the list
     * of atoms, it returns the index of each atom. If counters is a list of pairs
     * of indices (i.e. specifying pairs), for each pair of indices i,j it returns
     * the number entries in the list of pairs before i,j appears.
     */
    template<size_t Order>
    inline size_t get_offset_impl(const std::array<size_t, Order>
				  & counters) const;

    //! Returns the number of clusters of size cluster_size
    inline size_t get_nb_clusters(size_t cluster_size) const {
      switch (cluster_size) {
      case traits::MaxOrder: {
        return this->neighbours.size();
        break;
      }
      default:
        return this->manager.get_nb_clusters(cluster_size);
        break;
      }
    }

    //! Returns number of clusters of the original manager
    inline size_t get_size() const {
      //! return this->get_nb_clusters(1); TODO: this has to return the original
      //! number of atoms from the underlying manager return
      //! this->neighbours.size();
      return this->manager.get_size();
    }

    //! Returns position of an atom with index atom_index
    //! (useful for developers)
    inline Vector_ref get_position(const size_t & atom_index) {
      return this->manager.get_position(atom_index);
    }

    //! Returns position of the given atom object (useful for users)
    inline Vector_ref get_position(const AtomRef_t & atom) {
      return this->manager.get_position(atom.get_index());
    }

    //! TODO: Finish the implementation
    template<size_t Order, size_t Layer>
    inline Vector_ref get_neighbour_position(const ClusterRefKey<Order, Layer>&
                                             /*cluster*/) {
      static_assert(Order > 1,
                    "Only possible for Order > 1.");
      static_assert(Order < traits::MaxOrder,
                    "this implementation should only work up to MaxOrder.");
      //! Argument is now the same, but implementation
      throw std::runtime_error("should be adapted to Félix's "
                               "new interface using the ClusterRef");
    }

    //! Returns the id of the index-th (neighbour) atom of the cluster
    //! that is the full structure/atoms object, i.e. simply the id of
    //! the index-th atom
    inline int get_cluster_neighbour(const Parent& /*parent*/,
				     size_t index) const {
      return this->manager.get_cluster_neighbour(this->manager, index);
    }

    //! Returns the id of the index-th neighbour atom of a given cluster
    template<size_t Order, size_t Layer>
    inline int get_cluster_neighbour(const ClusterRefKey<Order, Layer>
				     & cluster,
				     size_t index) const {
      static_assert(Order < traits::MaxOrder,
                    "this implementation only handles up to traits::MaxOrder");
      if (Order < traits::MaxOrder-1) {
	return this->manager.get_cluster_neighbour(cluster, index);
      } else {
	auto && offset = this->offsets[cluster.get_cluster_index(Layer)];
	return this->neighbours[offset + index];
      }
    }


    //! Returns atom type given an atom object AtomRef
    inline int & get_atom_type(const AtomRef_t& atom) {
      return this->manager.get_atom_type(atom.get_index());
    }

    //! Returns a constant atom type given an atom object AtomRef
    inline const int & get_atom_type(const AtomRef_t& atom) const {
      return this->manager.get_atom_type(atom.get_index());
    }

    //! Returns the number of neighbors of a given cluster
    template<size_t Order, size_t Layer>
    inline size_t get_cluster_size(const ClusterRefKey<Order, Layer>
                                   & cluster) const {
      static_assert(Order < traits::MaxOrder,
                    "this implementation handles only the respective MaxOrder");
      if (Order < traits::MaxOrder-1) {
	return this->manager.get_cluster_size(cluster);
      } else {
        auto access_index = cluster.get_cluster_index(Layer);
	return nb_neigh[access_index];
      }
    }

    //! Returns the number of neighbors of an atom with the given index
    inline size_t get_cluster_size(const int & atom_index) const {
      return this->manager.get_cluster_size(atom_index);
    }

  protected:
    /**
     * Main function during construction of a neighbourlist.
     *
     * @param atom The atom to add to the list. Because the MaxOrder is
     * increased by one in this adaptor, the Order=MaxOrder
     */
    inline void add_atom(const int atom_index) {
      //! adds new atom at this Order
      this->atom_indices.push_back(atom_index);
      //! increases the number of neighbours
      this->nb_neigh.back()++;
      //! increases the offsets
      this->offsets.back()++;

      //! extends the list containing the number of neighbours
      //! with a new 0 entry for the added atom
      this->nb_neigh.push_back(0);
      //! extends the list containing the offsets and sets it
      //! with the number of neighbours plus the offsets of the last atom
      this->offsets.push_back(this->offsets.back() +
                              this->nb_neigh.back());
    }

    //! Extends the list containing the number of neighbours with a 0
    inline void add_entry_number_of_neighbours() {
      this->nb_neigh.push_back(0);
    }

    //! Adds a given atom index as new cluster neighbour
    inline void add_neighbour_of_cluster(const int atom_index) {
      //! adds `atom_index` to neighbours
      this->neighbours.push_back(atom_index);
      //! increases the number of neighbours
      this->nb_neigh.back()++;
    }

    //! Sets the correct offsets for accessing neighbours
    inline void set_offsets() {
      auto n_tuples{nb_neigh.size()};
      this->offsets.reserve(n_tuples);
      this->offsets.resize(1);
      // this->offsets.push_back(0);
      for (size_t i{0}; i < n_tuples; ++i) {
        this->offsets.emplace_back(this->offsets[i] + this->nb_neigh[i]);
      }
    }

    //! Interface of the add_atom function that adds the last atom in a
    //! given cluster
    template <size_t Order>
    inline void add_atom(const typename ManagerImplementation::template
                         ClusterRef<Order> & cluster) {
      static_assert(Order <= traits::MaxOrder,
                    "Order too high, not possible to add atom");
      return this->add_atom(cluster.back());
    }

    //TODO(federico.giberti@gmail.com): finish the documentation

    //! Makes a half neighbour list, by construction only Order=1 is supplied.
    void make_half_neighbour_list();

    //! full neighbour list
    void make_full_neighbour_list();

    //! find the corresponding cell indices for all atom positions
    void make_cells_for_neighbourlist();

    // //! Makes a full neighbour list
    // void make_full_neighbour_list();

    // //! Increases whatever body order is present
    // void increase_maxorder();

    ManagerImplementation & manager;

    //! Cutoff radius of manager
    const double cutoff;

    template<size_t Order, bool IsDummy> struct AddOrderLoop;

    /**
     * Compile time decision, if a new neighbour list is built or if an existing
     * one is extended.
     */
    template <size_t Order, bool IsDummy> struct IncreaseMaxOrder;

    //! not necessary any more
    //! // stores AtomRefs to of neighbours for traits::MaxOrder-1-*plets
    // std::vector<AtomRef_t> atom_refs{};

    //! Stores atom indices of current Order
    std::vector<size_t> atom_indices{}; //akin to ilist[]

    //! Stores the number of neighbours for every traits::MaxOrder-1-*plets
    std::vector<size_t> nb_neigh{};

    //! Stores all neighbours of traits::MaxOrder-1-*plets
    std::vector<size_t> neighbours{};

    //! Stores the offsets of traits::MaxOrder-1-*plets for accessing
    //! `neighbours`, from where nb_neigh can be counted
    std::vector<size_t> offsets{};

    size_t cluster_counter{0};
  private:
  };



  namespace internal {

    // conversion of a linear id to a Dim_t id
    template<int Dim>
    inline void
    linear_to_dim_index(const Dim_t& index,
                        const Eigen::Ref<const Vec3i_t> shape,
                        Eigen::Ref< Vec3i_t> retval) {
      Dim_t factor{1};

      for (Dim_t i{0}; i < Dim; ++i) {
        retval[i] = index / factor%shape[i];
        if (i != Dim-1) {
          factor *= shape[i];
        }
      }
    }

    template<int Dim>
    inline Dim_t
    dimension_to_linear_index(const Eigen::Ref<const Vec3i_t> coord,
                              const Eigen::Ref<const Vec3i_t> shape) {
      Dim_t index{0};
      Dim_t factor{1};
      for (Dim_t i = 0; i < Dim; ++i) {
        index += coord[i]*factor;
        if (i != Dim-1 ) {
          factor *= shape[i];
        }
      }
      return index;
    }

    //! Taken from StructureManagerCell
    // https://stackoverflow.com/questions/828092/python-style-integer-division-modulus-in-c
    // TODO more efficient implementation without if (would be less general) !
    // div_mod function returning python like div_mod, i.e. signed integer
    // division truncates towards negative infinity, and signed integer modulus
    // has the same sign the second operand.

    template<class T>
    decltype(auto) modulo_and_rest(const T & x, const T & y) {
      std::array<int, 2> out;
      const T quot = x / y;
      const T rem  = x % y;

      if (rem != 0 && (x<0) != (y<0)) {
        out[0] = quot-1;
        out[1] = rem+y;
      } else {
        out[0] = quot;
        out[1] = rem;
      }

      return out;
    }

  }  // internal

  //----------------------------------------------------------------------------//
  // TODO include a distinction for the cutoff: with respect to the
  // i-atom only or with respect to the j,k,l etc. atom. I.e. the
  // cutoff goes with the Order.
  template <class ManagerImplementation>
  AdaptorMaxOrder<ManagerImplementation>::
  AdaptorMaxOrder(ManagerImplementation & manager, double cutoff):
    manager{manager},
    cutoff{cutoff},
    atom_indices{},
    nb_neigh{},
    offsets{}

  {
    if (traits::MaxOrder < 1) {
      throw std::runtime_error("No atoms in manager.");
    }
  }

  /* ---------------------------------------------------------------------- */
  template <class ManagerImplementation>
  template <class ... Args>
  void AdaptorMaxOrder<ManagerImplementation>::update(Args&&... arguments) {
    this->manager.update(std::forward<Args>(arguments)...);
    this->update();
  }

  /* ---------------------------------------------------------------------- */
  template <class ManagerImplementation>
  template <size_t Order, bool IsDummy>
  struct AdaptorMaxOrder<ManagerImplementation>::AddOrderLoop {
    static constexpr int OldMaxOrder{ManagerImplementation::traits::MaxOrder};
    using ClusterRef_t = typename ManagerImplementation::template
      ClusterRef<Order>;

    using NextOrderLoop = AddOrderLoop<Order+1,
				       (Order+1 == OldMaxOrder)>;

    static void loop(ClusterRef_t & cluster,
                     AdaptorMaxOrder<ManagerImplementation> & manager) {

      //! do nothing, if MaxOrder is not reached, except call the next order
      for (auto next_cluster : cluster) {

        auto & next_cluster_indices
        {std::get<Order>(manager.cluster_indices_container)};
        next_cluster_indices.push_back(next_cluster.get_cluster_indices());

	NextOrderLoop::loop(next_cluster, manager);
      }
    }
  };

  /* ---------------------------------------------------------------------- */
  //! At desired MaxOrder (plus one), here is where the magic happens and the
  //! neighbours of the same order are added as the Order+1.  add check for non
  //! half neighbour list
  template <class ManagerImplementation>
  template <size_t Order>
  struct AdaptorMaxOrder<ManagerImplementation>::AddOrderLoop<Order, true> {
    static constexpr int OldMaxOrder{ManagerImplementation::traits::MaxOrder};

    using ClusterRef_t =
      typename ManagerImplementation::template ClusterRef<Order>;

    // using Manager_t = StructureManager<ManagerImplementation>;
    // using IteratorOne_t = typename Manager_t::template iterator<1>;

    using traits = typename AdaptorMaxOrder<ManagerImplementation>::traits;

    static void loop(ClusterRef_t & cluster,
                     AdaptorMaxOrder<ManagerImplementation> & manager) {

      //! get all i_atoms to find neighbours to extend the cluster to the next
      //! order
      auto i_atoms = cluster.get_atom_indices();

      //! vector of existing i_atoms in `cluster` to avoid doubling of atoms in
      //! final list
      std::vector<size_t> current_i_atoms;
      //! a set of new neighbours for the cluster, which will be added to extend
      //! the cluster
      std::set<size_t> current_j_atoms;

      //! access to underlying manager for access to atom pairs
      auto & manager_tmp{cluster.get_manager()};

      // std::cout << " === neigh back "
      //           << i_atoms[0] << " " << i_atoms[1]
      //           << std::endl;

      for (auto atom_index : i_atoms) {
        current_i_atoms.push_back(atom_index);
        size_t access_index = manager.get_cluster_neighbour(manager,
                                                            atom_index);

        //! build a shifted iterator to constuct a ClusterRef<1>
        auto iterator_at_position{manager_tmp.get_iterator_at(access_index)};

        //! ClusterRef<1> as dereference from iterator to get pairs of the
        //! i_atoms
        auto && j_cluster{*iterator_at_position};

        //! collect all possible neighbours of the cluster: collection of all
        //! neighbours of current_i_atoms
        for (auto pair : j_cluster) {
          auto j_add = pair.back();
          if (j_add > i_atoms.back()) {
            current_j_atoms.insert(j_add);
          }
        }
      }

      //! delete existing cluster atoms from list to build additional neighbours
      std::vector<size_t> atoms_to_add{};
      std::set_difference(current_j_atoms.begin(), current_j_atoms.end(),
                          current_i_atoms.begin(), current_i_atoms.end(),
                          std::inserter(atoms_to_add, atoms_to_add.begin()));

      manager.add_entry_number_of_neighbours();
      if (atoms_to_add.size() > 0) {
        for (auto j: atoms_to_add) {
          manager.add_neighbour_of_cluster(j);
        }
      }
    }
  };

  /* ---------------------------------------------------------------------- */
  template <class ManagerImplementation>
  void AdaptorMaxOrder<ManagerImplementation>::make_half_neighbour_list() {
    //! Make a half neighbour list (not quite Verlet, because of the missing
    //! skin) according to Tadmor and Miller 'Modeling Materials', algorithm 6.7,
    //! p 323, (needs modification for periodicity).

    //! It results in a <code>strict</code> neighbourlist. It is only a
    //! half-neighbour list.  The most obvious difference is that no 'skin' is
    //! used in conjunction with the cutoff.  This is only necessary, if the
    //! ManagerImplementation, with which this adaptor is initialized does not
    //! have at least already atomic pairs. Inluding ghost neighbours?

    // TODO: add functionality for the shift vector??!

    // TODO: include linked-list-cell algorithm from Felix _cell.cc here before
    // the building of the neighbour list
    // Add a distinction between full and half neighbour list -- probably in traits?

    //! The zeroth order does not have neighbours

    this->nb_neigh.push_back(0);
    this->offsets.push_back(0);

    unsigned int nneigh_off{0};

    for (auto it=this->manager.begin(); it!=--this->manager.end(); ++it){
      //! Add atom at this order this is just the standard list.
      auto atom_i = *it;
      this->add_atom(atom_i);

      auto jt = it;
      ++jt; //! go to next atom in manager (no self-neighbour)
      for (;jt!=manager.end(); ++jt){
      	auto atom_j = *jt;
      	double distance{(atom_i.get_position() -
			 atom_j.get_position()).norm()};
      	if (distance <= this->cutoff) {
      	  //! Store atom_j in neighbourlist of atom_i
          this->neighbours.push_back(atom_j.get_index());
      	  this->nb_neigh.back()++;
      	  nneigh_off += 1;
      	}
      }
      this->offsets.push_back(nneigh_off);
    }
    // get the cluster_indices right

    auto & atom_cluster_indices{std::get<0>(this->cluster_indices_container)};
    atom_cluster_indices.fill_sequence();
    auto & pair_cluster_indices{std::get<1>(this->cluster_indices_container)};
    pair_cluster_indices.fill_sequence();
  }

  /* ---------------------------------------------------------------------- */
  /**
   * Make a full neighbour list, transferred from former StructureManagerCell,
   * slightly modified to fit the current member variables.
   */
  template <class ManagerImplementation>
  void AdaptorMaxOrder<ManagerImplementation>::make_full_neighbour_list() {

    size_t natoms{this->get_manager().size()};

    const auto dim{traits::Dim};

    std::vector<AtomRef_t> centers;
    std::vector<double> positions;
  std:vector<int> particle_types;
    Lattice lattice;

    for (auto atom : this->get_manager()) {
      centers.push_back(AtomRef_t(this->get_manager(), atom.get_index()));
      positions.push_back(atom.get_position());
      particle_types.push_back(atom.get_atom_type());
    }

    Cell_t lat = cell;
    lattice.set_cell(lat);
    cell = lat;
    Vec3_t reciprocal_lenghts = this->lattice.get_reciprocal_lenghts();
    double bin_size{cutoff_max};
    Vec3i_t nbins_c,neigh_search;
    Vec3_t nbins_cd;
    int nbins{1};
    double face_dist_c;


    for (int ii{0};ii<dim;++ii){
      // compute the distance between the cell faces (only French wiki
      // https://fr.wikipedia.org/wiki/Distance_interr%C3%A9ticulaire)
      if (reciprocal_lenghts[ii] > 0){
        face_dist_c = 1 / reciprocal_lenghts[ii];
      }
      else {
        face_dist_c = 1;
      }
      // number of bin in each directions
      nbins_c[ii] =  std::max( static_cast<int>(face_dist_c/bin_size), 1);
      nbins_cd[ii] = static_cast<double>(nbins_c[ii]);
      // number of bin one need to look around
      neigh_search[ii] = static_cast<int>(std::
                                          ceil(bin_size * nbins_c[ii]
                                               / face_dist_c));
      // total number of bin
      nbins *= nbins_c[ii];

    }

    Vec3i_t bin_index_c;
    for (int ii{0}; ii < nbins; ++ii){
      internal::lin2mult<dim>(ii,nbins_c, bin_index_c);
      this->boxes.push_back(Box(this->get_manager(), bin_index_c,
                                pbc, neigh_search, nbins_c));
    }

    // bin the particles in the boxes
    Vec3_t position_sc;
    int bin_id{0};
    this->part2bin.resize(Natom);
    for (auto part : this->particles){
      this->lattice.get_cartesian2scaled(part.get_position(), position_sc);
      bin_index_c = (position_sc.array() * nbins_cd.array()).cast<int>();
      bin_id = internal::mult2lin<dim>(bin_index_c, nbins_c);
      this->boxes[bin_id].push_particle_back(part.get_index());
      this->part2bin[part.get_index()] = bin_id;
    }

    // Set up the data strucure containing the information about neighbourhood
    // get the number of particles in the box and its neighbour boxes set the
    // arrays that will be used to iterate over the centers and neighbourr
    //
    // TODO: short circuit for building neighbours, nneigh, etc. to make calls in
    // manager short
    this->neighbour_bin_id.resize(nbins);
    this->neighbour_atom_index.resize(nbins);
    //loop over the boxes
    for (size_t bin_index{0}; bin_index < this->boxes.size(); ++bin_index){
      size_t n_neigh{0};
      // loop over the neighbouring boxes
      for (size_t neigh_bin_id{0}; neigh_bin_id <
             this->boxes[bin_index].get_number_of_neighbour_box();
           ++neigh_bin_id){
        int neig_bin_index{
          this->boxes[bin_index].get_neighbour_bin_index(neigh_bin_id)
            };
        //loop over the particle in the neighbouring boxes
        for (size_t neigh_part_id{0}; neigh_part_id
               < this->boxes[neig_bin_index].get_number_of_particles();
             ++neigh_part_id){
          // store the indices to the corresponding atomic shift
          this->neighbour_bin_id[bin_index].push_back(AtomRef_t(this->get_manager(),
                                                                neigh_bin_id));
          // store the indices to the neighbour particles
          // TODO: basically a neighbour list??
          this->neighbour_atom_index[bin_index]
            .push_back(AtomRef_t(this->get_manager(),
                                 this->boxes[neig_bin_index]
                                 .get_particle_index(neigh_part_id)));
        }
        n_neigh += this->boxes[neig_bin_index].get_number_of_particles();
      }
      this->boxes[bin_index].set_number_of_neighbours(n_neigh);
    }

    int stride{0};
    // get the stride for the fields. (center,neigh) dimensions are flattened in
    // fields with center being leading dimension
    // TODO: number_of_neighbours; offsets can be calculated from this
    for (auto center : this->centers){
      this->number_of_neighbours_stride.push_back(stride);
      int bin_index{this->part2bin[center.get_index()]};
      size_t n_neigh{this->boxes[bin_index].get_number_of_neighbours()};
      stride += n_neigh;
    }
    this->number_of_neighbours = stride;
  }


  /* ---------------------------------------------------------------------- */
  //! Extend and existing neighbour list.
  template <class ManagerImplementation>
  template <size_t MaxOrder, bool IsDummy>
  struct AdaptorMaxOrder<ManagerImplementation>::IncreaseMaxOrder {

    static void increase_maxorder(AdaptorMaxOrder<ManagerImplementation>
                                  * manager_max) {

      static_assert(MaxOrder > 2,
                    "no neighbourlist present; extension not possible.");

      for (auto atom : manager_max->manager) {
        //! Order 1, Order variable is at 0, atoms, index 0
        using AddOrderLoop = AddOrderLoop<atom.order(),
                                          atom.order() == traits::MaxOrder-1>;
        auto & atom_cluster_indices{std::get<0>
            (manager_max->cluster_indices_container)};
        atom_cluster_indices.push_back(atom.get_cluster_indices());
        AddOrderLoop::loop(atom, *manager_max);
      }

      //! correct the offsets for the new cluster order
      manager_max->set_offsets();
      //! add correct cluster_indices for the highest order
      auto & max_cluster_indices
      {std::get<traits::MaxOrder-1>(manager_max->cluster_indices_container)};
      max_cluster_indices.fill_sequence();
    }
  };

  template <class ManagerImplementation>
  template <size_t MaxOrder>
  struct AdaptorMaxOrder<ManagerImplementation>::
  IncreaseMaxOrder<MaxOrder, true> {
    static void increase_maxorder(AdaptorMaxOrder<ManagerImplementation>
                                  * manager_max) {
      manager_max->nb_neigh.resize(0);
      manager_max->offsets.resize(0);
      manager_max->neighbours.resize(0);
      manager_max->make_half_neighbour_list();
      manager_max->make_full_neighbour_list();
    }
  };


  template <class ManagerImplementation>
  void AdaptorMaxOrder<ManagerImplementation>::update() {
    /**
     * Standard case, increase an existing neighbour list or triplet list to a
     * higher Order
     */
    using IncreaseMaxOrder = IncreaseMaxOrder<traits::MaxOrder,
                                              (traits::MaxOrder==2)>;
    IncreaseMaxOrder::increase_maxorder(this);
  }

  /* ---------------------------------------------------------------------- */
  /* Returns the linear indices of the clusters (whose atom indices
   * are stored in counters). For example when counters is just the list
   * of atoms, it returns the index of each atom. If counters is a list of pairs
   * of indices (i.e. specifying pairs), for each pair of indices i,j it returns
   * the number entries in the list of pairs before i,j appears.
   */
  template<class ManagerImplementation>
  template<size_t Order>
  inline size_t AdaptorMaxOrder<ManagerImplementation>::
  get_offset_impl(const std::array<size_t, Order> & counters) const {

    static_assert(Order < traits::MaxOrder,
                  "this implementation handles only up to the respective MaxOrder");
    /**
     * Order accessor: 0 - atoms
     *                 1 - pairs
     *                 2 - triplets
     *                 etc.
     * Order is determined by the ClusterRef building iterator, not by the Order
     * of the built iterator
     */
    if (Order == 1) {
      return this->offsets[counters.front()];
    } else if (Order == traits::MaxOrder-1) {
      /**
       * Counters as an array to call parent offset multiplet. This can then be
       * used to access the actual offset for the next Order here.
       */
      auto i{this->manager.get_offset_impl(counters)};
      auto j{counters[Order-1]};
      auto tuple_index{i+j};
      auto main_offset{this->offsets[tuple_index]};
      return main_offset;
    } else {
      /**
       * If not accessible at this order, call lower Order offsets from lower
       * order manager(s).
       */
      return this->manager.get_offset_impl(counters);
    }
  }
}  // rascal

#endif /* ADAPTOR_MAXORDER_H */

// TODO: The construction of triplets is fine, but they occur multiple times. We
// probably need to check for increasing atomic index to get rid of
// duplicates. But this is in general a design decision, if we want full/half neighbour list and full/half/whatever triplets and quadruplets
