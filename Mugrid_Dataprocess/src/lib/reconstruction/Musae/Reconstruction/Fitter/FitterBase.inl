namespace Musae::inline Reconstruction::inline Fitter {

template<Mustard::Data::SuperTupleModel<Data::LGAHit> AHit,
         Mustard::Data::SuperTupleModel<Data::CRMuEvent> AEvent>
FitterBase<AHit, AEvent>::FitterBase() :
    fMinNHit{Detector::Description::LGA::Instance().NHitThreshold()} {}

} // namespace Musae::inline Reconstruction::inline Fitter
