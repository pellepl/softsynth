play: sound.raw
	sox -V -t raw -e unsigned-integer -b 8 -c 1 -r 32768 sound.raw -d

sound.raw: soft_synth
	./soft_synth sound.raw

soft_synth:
	gcc -Isrc -o soft_synth src/audio_test.c src/soft_synth.c src/tune.c

clean:
	rm -rf soft_synth sound.raw

