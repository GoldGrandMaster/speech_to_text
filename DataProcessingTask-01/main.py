import ctranslate2
import numpy as np

"""
This is the function that you will need to implement correctly in cpp
"""
def get_ctranslate2_storage(segment: np.ndarray) -> ctranslate2.StorageView:
    segment = segment.astype(np.float32)
    segment = np.ascontiguousarray(segment)
    segment = np.expand_dims(segment, 0)
    segment = ctranslate2.StorageView.from_array(segment)
    return segment


if __name__ == "__main__":
    # let's start by defining the model
    model_path = "C:/dev/Resources/Models/whisper-tiny.en-ct2"
    model = ctranslate2.models.Whisper(model_path)

    # here we extract we data from the csv file
    features = np.genfromtxt('D:/Git/DataProcessingTask-01/audio_test.csv', delimiter=',')

    # we follow with processing the data correctly, showing that it is possible and works in python
    features = get_ctranslate2_storage(features)

    # perform inference and set some params
    result = model.generate(
        features,
        [[50257]],
        length_penalty=1,
        max_length=448,
        return_scores=True,
        return_no_speech_prob=True,
        suppress_blank=True,
        max_initial_timestamp_index=50,
        beam_size=1,
        patience=1
    )[0]

    # testing our system
    print(result.sequences[0][1])

    if result.sequences[0][1] == 'ĠHello':
        print("Congrats the python test works!")
    else:
        print("Ohh no something went wrong!")
