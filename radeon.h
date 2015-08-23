/****************************************************/
/* Graphics Object ID Bit definition                */
/****************************************************/
#define OBJECT_ID_MASK                            0x00FF
#define ENUM_ID_MASK                              0x0700
#define RESERVED1_ID_MASK                         0x0800
#define OBJECT_TYPE_MASK                          0x7000
#define RESERVED2_ID_MASK                         0x8000

#define OBJECT_ID_SHIFT                           0x00
#define ENUM_ID_SHIFT                             0x08
#define OBJECT_TYPE_SHIFT                         0x0C

enum radeon_hpd_id {
	RADEON_HPD_1 = 0,
	RADEON_HPD_2,
	RADEON_HPD_3,
	RADEON_HPD_4,
	RADEON_HPD_5,
	RADEON_HPD_6,
	RADEON_HPD_NONE = 0xff,
};

typedef u_char bool;
typedef uint32_t u32;
struct radeon_i2c_bus_rec {
	bool valid;
	/* id used by atom */
	uint8_t i2c_id;
	/* id used by atom */
	enum radeon_hpd_id hpd;
	/* can be used with hw i2c engine */
	bool hw_capable;
	/* uses multi-media i2c engine */
	bool mm_i2c;
	/* regs and bits */
	uint32_t mask_clk_reg;
	uint32_t mask_data_reg;
	uint32_t a_clk_reg;
	uint32_t a_data_reg;
	uint32_t en_clk_reg;
	uint32_t en_data_reg;
	uint32_t y_clk_reg;
	uint32_t y_data_reg;
	uint32_t mask_clk_mask;
	uint32_t mask_data_mask;
	uint32_t a_clk_mask;
	uint32_t a_data_mask;
	uint32_t en_clk_mask;
	uint32_t en_data_mask;
	uint32_t y_clk_mask;
	uint32_t y_data_mask;
};
struct radeon_gpio_rec {
	bool valid;
	u8 id;
	u32 reg;
	u32 mask;
};


struct radeon_hpd {
	enum radeon_hpd_id hpd;
	u8 plugged_state;
	struct radeon_gpio_rec gpio;
};

struct radeon_router {
	u32 router_id;
	struct radeon_i2c_bus_rec i2c_info;
	u8 i2c_addr;
	/* i2c mux */
	bool ddc_valid;
	u8 ddc_mux_type;
	u8 ddc_mux_control_pin;
	u8 ddc_mux_state;
	/* clock/data mux */
	bool cd_valid;
	u8 cd_mux_type;
	u8 cd_mux_control_pin;
	u8 cd_mux_state;
};

