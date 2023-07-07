#ifndef COMMON_H_
#define COMMON_H_


/* Number of samples of buffer to be processed */
#define N_SAMPLES		(128*2)


enum  Effects_type {
    TREMOLO,
    DISTORTION,
    BITCRUSHER,
    FLANGER
};


struct Effects_config {
    //Effects_t type;
    int status;
    float values[10];
    int n_args;
    int height;
    int pos_y;
};

#define NUMBER_OF_EFFECTS   (4)
#define PLOT_DECIMATION     (2)

#endif /* COMMON_H_ */

