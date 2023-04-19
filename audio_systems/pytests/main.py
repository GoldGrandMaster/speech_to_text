from audio import decode_audio


def test_decode():
	audio = decode_audio("../assets/Recording.wav")
	print(audio.shape == (164864,))

if __name__ == "__main__":
	test_decode()
