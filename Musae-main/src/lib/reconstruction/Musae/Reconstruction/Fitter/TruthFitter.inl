namespace Musae::inline Reconstruction::inline Fitter {

template<Mustard::Data::SuperTupleModel<Data::LGASimHit> AHit,
         Mustard::Data::SuperTupleModel<Data::CRMuSimEvent> AEvent>
template<std::indirectly_readable AHitPointer>
    requires Mustard::Data::SuperTupleModel<typename std::iter_value_t<AHitPointer>::Model, AHit>
auto TruthFitter<AHit, AEvent>::operator()(const std::vector<AHitPointer>& hitData) -> std::shared_ptr<Mustard::Data::Tuple<AEvent>> {
    if (ssize(hitData) < this->MinNHit()) {
        return {};
    }

    const auto& firstHit{*hitData.front()};
    const auto p{*Get<"p">(firstHit)};
    float totalEdep{0.0};

    for (const auto& hitPtr : hitData) {
        totalEdep += Get<"Edep">(*hitPtr); 
    }

    const auto event{std::make_shared_for_overwrite<Mustard::Data::Tuple<AEvent>>()};
    Get<"EvtID">(*event) = Get<"EvtID">(firstHit);
    Get<"DetID">(*event) = Get<"DetID">(firstHit);
    Get<"HitID">(*event)->reserve(hitData.size());
    for (auto&& hit : hitData) { Get<"HitID">(*event)->emplace_back(Get<"HitID">(*hit)); }
    Get<"t0">(*event) = Get<"t">(firstHit);
    Get<"x0">(*event) = Get<"x">(firstHit);
    Get<"theta">(*event) = std::atan(std::hypot(p[0], p[1]) / -p[2]);
    Get<"phi">(*event) = std::atan2(-p[1], -p[0]);
    Get<"Edep">(*event) = totalEdep;
    Get<"TrkID">(*event) = Get<"TrkID">(firstHit);
    Get<"PDGID">(*event) = Get<"PDGID">(firstHit);
    Get<"w">(*event) = Get<"w">(firstHit);
    Get<"Ek0">(*event) = Get<"Ek">(firstHit);
    Get<"p0">(*event) = Get<"p">(firstHit);

    if (not fCheckHitDataConsistency) { return event; }

#define MUSAE_RECONSTRUCTION_FITTER_TRUTH_FITTER_HIT_DATA_CONSISTENCY_CHECK(cond) \
    if (cond) {                                                                   \
        Mustard::PrintError(#cond);                                               \
    }
    for (auto&& hit : hitData) {
        MUSAE_RECONSTRUCTION_FITTER_TRUTH_FITTER_HIT_DATA_CONSISTENCY_CHECK(Get<"EvtID">(*hit) != Get<"EvtID">(firstHit))
        MUSAE_RECONSTRUCTION_FITTER_TRUTH_FITTER_HIT_DATA_CONSISTENCY_CHECK(Get<"TrkID">(*hit) != Get<"TrkID">(firstHit))
        MUSAE_RECONSTRUCTION_FITTER_TRUTH_FITTER_HIT_DATA_CONSISTENCY_CHECK(Get<"PDGID">(*hit) != Get<"PDGID">(firstHit))
    }
#undef MUSAE_RECONSTRUCTION_FITTER_TRUTH_FITTER_HIT_DATA_CONSISTENCY_CHECK

    return event;
}

} // namespace Musae::inline Reconstruction::inline Fitter
