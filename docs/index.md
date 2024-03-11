---
hide:
  - title
---
<h1 align="center"><code>pd-simpl</code></h1>


<p align="center">
    Welcome to the <code>pd-simpl</code> docs. 
</p>

---

`pd-simpl` is a library that allows the use of Partial Tracking algorithms in realtime. It uses the `C++` code developed by John Glover an very well presented in the thesis: `Sinusoids, noise and transients`, you can read it [here](https://mural.maynoothuniversity.ie/4523/1/thesis.pdf) (I really recommend!).

## Install 
!!! warning
    You need to use `declare -lib simpl` for these objects be available in PureData or PlugData.

## Objects

With `pd-simpl` you will have four objects. 

* `s-peaks~`: Peaks Detections and Partial Tracking; 
* `s-synth~`: Synthesis.
* `s-get`: Get numbers of the frequency, amplitude, phase and bandwidth of each partials.
* `s-create`: Using frequency, amplitude, phase and bandwith you can create partials.
* `s-trans`: Allows transformations with the partials. Silence some of the partials, transpose, amplitude alterations, and others.

---

### `s-peaks~`

`s-peaks~` is the object used to make the `Peak Detection` and the `Partial Tracking`. You can select four methods for each process: `sms`, `loris`, `sndobj` and `mq`. To select `loris` for `Peak Detection`, for example, you send the message `set peak loris`. For `sms` the message is `set peak sms`.

<p align="center">
        <img src="assets/s-peaks.png" width="50%" alt="Help Patch for s-peaks"  style="box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.2);">
</p>



#### Options

##### `All`
* `maxpeaks`: Set the maximum number of Peaks and Partials.
* `framesize`: Set the Analysis FrameSize;
* `hopsize`: Set the Analysis HopSize
* `set peak`: Set the method for Peak Detection;
* `set partial`: Set the method for Partial Tracking;

##### `Sms` 

When `Sms` Partial Tracking is select you have extra options:

* `set harmonic`: Set the analysis, it can be `Harmonic Partial Tracking without Phase`, `Inharmonic Partial Tracking without Phase`, `Harmonic Partial Tracking with Phase` and `Inharmonic Partial Tracking with Phase`.



---

### `s-synth~`


---

### `s-get`

---

### `s-create`


---

### `s-trans`


## You need help?

If you need help, has some doubt, or found an error please use [Discussions](https://github.com/charlesneimog/pd-simpl/discussions) for ask things and [Issues](https://github.com/charlesneimog/pd-simpl/issues) to report errors. 

If you don't want to use Github, send me an e-mail in [charlesneimog@outlook.com](malito:charlesneimog@outlook.com). 
