---
hide:
  - title
---

### `s-peaks~`

`s-peaks~` is the object used to make the `Peak Detection` and the `Partial Tracking`. You can select four methods for each process: `sms`, `loris`, `sndobj` and `mq`. To select `loris` for `Peak Detection`, for example, you send the message `set peak loris`. For `sms` the message is `set peak sms`.

<p align="center">
        <img src="assets/s-peaks.png" width="50%" alt="Help Patch for s-peaks"  style="box-shadow: 0px 4px 8px rgba(0, 0, 0, 0.2);">
</p>



#### Options

##### `All`

* `set maxpeaks`: Set the maximum number of Peaks and Partials.
* `set framesize`: Set the Analysis FrameSize;
* `set hopsize`: Set the Analysis HopSize;
* `set peak`: Set the method for Peak Detection;
* `set partial`: Set the method for Partial Tracking;

--- 

##### `Sms` 

When `Sms` Partial Tracking is select you have extra options:

* `set harmonic`: Set the analysis, it can be: 
    * `Harmonic Partial Tracking without Phase`; 
    * `Inharmonic Partial Tracking without Phase`;
    * `Harmonic Partial Tracking with Phase`;
    * `Inharmonic Partial Tracking with Phase`.

* `set fundamental`: Set the default fundamental pitch;


---pl
