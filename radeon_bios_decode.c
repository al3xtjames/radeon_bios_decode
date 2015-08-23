#include <sys/types.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "atombios.h"
#include "radeon_bios_decode.h"
/*#include <byteswap.h>*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define X_BIG_ENDIAN 1
#define X_LITTLE_ENDIAN 2
#define X_BYTE_ORDER X_LITTLE_ENDIAN
#if X_BYTE_ORDER == X_BIG_ENDIAN
asdf
#define le32_to_cpu(x) bswap_32(x)
#define le16_to_cpu(x) bswap_16(x)
#define cpu_to_le32(x) bswap_32(x)
#define cpu_to_le16(x) bswap_16(x)
#else
#define le32_to_cpu(x) (x)
#define le16_to_cpu(x) (x)
#define cpu_to_le32(x) (x)
#define cpu_to_le16(x) (x)
#endif


#define DRM_MODE_CONNECTOR_Unknown	0
#define DRM_MODE_CONNECTOR_VGA		1
#define DRM_MODE_CONNECTOR_DVII		2
#define DRM_MODE_CONNECTOR_DVID		3
#define DRM_MODE_CONNECTOR_DVIA		4
#define DRM_MODE_CONNECTOR_Composite	5
#define DRM_MODE_CONNECTOR_SVIDEO	6
#define DRM_MODE_CONNECTOR_LVDS		7
#define DRM_MODE_CONNECTOR_Component	8
#define DRM_MODE_CONNECTOR_9PinDIN	9
#define DRM_MODE_CONNECTOR_DisplayPort	10
#define DRM_MODE_CONNECTOR_HDMIA	11
#define DRM_MODE_CONNECTOR_HDMIB	12
#define DRM_MODE_CONNECTOR_TV		13
#define DRM_MODE_CONNECTOR_eDP		14

const int object_connector_convert[] = {
	DRM_MODE_CONNECTOR_Unknown,
	DRM_MODE_CONNECTOR_DVII,
	DRM_MODE_CONNECTOR_DVII,
	DRM_MODE_CONNECTOR_DVID,
	DRM_MODE_CONNECTOR_DVID,
	DRM_MODE_CONNECTOR_VGA,
	DRM_MODE_CONNECTOR_Composite,
	DRM_MODE_CONNECTOR_SVIDEO,
	DRM_MODE_CONNECTOR_Unknown,
	DRM_MODE_CONNECTOR_Unknown,
	DRM_MODE_CONNECTOR_9PinDIN,
	DRM_MODE_CONNECTOR_Unknown,
	DRM_MODE_CONNECTOR_HDMIA,
	DRM_MODE_CONNECTOR_HDMIB,
	DRM_MODE_CONNECTOR_LVDS,
	DRM_MODE_CONNECTOR_9PinDIN,
	DRM_MODE_CONNECTOR_Unknown,
	DRM_MODE_CONNECTOR_Unknown,
	DRM_MODE_CONNECTOR_Unknown,
	DRM_MODE_CONNECTOR_DisplayPort,
	DRM_MODE_CONNECTOR_eDP,
	DRM_MODE_CONNECTOR_Unknown
};

static const char *encoder_names[34] = {
	"NONE",
	"INTERNAL_LVDS",
	"INTERNAL_TMDS1",
	"INTERNAL_TMDS2",
	"INTERNAL_DAC1",
	"INTERNAL_DAC2",
	"INTERNAL_SDVOA",
	"INTERNAL_SDVOB",
	"SI170B",
	"CH7303",
	"CH7301",
	"INTERNAL_DVO1",
	"EXTERNAL_SDVOA",
	"EXTERNAL_SDVOB",
	"TITFP513",
	"INTERNAL_LVTM1",
	"VT1623",
	"HDMI_SI1930",
	"HDMI_INTERNAL",
	"INTERNAL_KLDSCP_TMDS1",
	"INTERNAL_KLDSCP_DVO1",
	"INTERNAL_KLDSCP_DAC1",
	"INTERNAL_KLDSCP_DAC2",
	"SI178",
	"MVPU_FPGA",
	"INTERNAL_DDI",
	"VT1625",
	"HDMI_SI1932",
	"DP_AN9801",
	"DP_DP501",
	"INTERNAL_UNIPHY",
	"INTERNAL_KLDSCP_LVTMA",
	"INTERNAL_UNIPHY1",
	"INTERNAL_UNIPHY2",
};

const char *output_names[] = { "None",
			       "VGA",
			       "DVI-I",
			       "DVI-D",
			       "DVI-A",
			       "Composite",
			       "S-video",
			       "LVDS",
			       "Component",
			       "9 pin DIN",
			       "DisplayPort",
			       "HDMI-A",
			       "HDMI-B",
			       "TV",
			       "eDP"
};

const char *
connectortype_to_name(uint32_t connector_type)
{
    if (connector_type > 14) {
	return NULL;
    }
    return(output_names[connector_type]);
}


static int
rhdAtomAnalyzeCommonHdr(ATOM_COMMON_TABLE_HEADER *hdr)
{
    if (le16_to_cpu(hdr->usStructureSize) == 0xaa55)
        return FALSE;

    return TRUE;
}

static int
rhdAtomAnalyzeRomHdr(unsigned char *rombase,
		     ATOM_ROM_HEADER *hdr,
		     unsigned int *data_offset, 
		     unsigned int *command_offset)
{
    if (!rhdAtomAnalyzeCommonHdr(&hdr->sHeader)) {
        return FALSE;
    }
    printf("\tSubsystemVendorID: 0x%4.4x SubsystemID: 0x%4.4x\n",
               le16_to_cpu(hdr->usSubsystemVendorID),le16_to_cpu(hdr->usSubsystemID));
    printf("\tIOBaseAddress: 0x%4.4x\n",le16_to_cpu(hdr->usIoBaseAddress));
    printf("\tFilename: %s\n",rombase + le16_to_cpu(hdr->usConfigFilenameOffset));
    printf("\tBIOS Bootup Message: %s\n",
		   rombase + le16_to_cpu(hdr->usBIOS_BootupMessageOffset));

    *data_offset = le16_to_cpu(hdr->usMasterDataTableOffset);
    *command_offset = le16_to_cpu(hdr->usMasterCommandTableOffset);

    return TRUE;
}

static int
rhdAtomAnalyzeRomDataTable(unsigned char *base, uint16_t offset,
                    void *ptr,unsigned short *size)
{
    ATOM_COMMON_TABLE_HEADER *table = (ATOM_COMMON_TABLE_HEADER *)
      (base + le16_to_cpu(offset));

   if (!*size || !rhdAtomAnalyzeCommonHdr(table)) {
       if (*size) *size -= 2;
       *(void **)ptr = NULL;
       return FALSE;
   }
   *size -= 2;
   *(void **)ptr = (void *)(table);
   return TRUE;
}

Bool
rhdAtomGetTableRevisionAndSize(ATOM_COMMON_TABLE_HEADER *hdr,
			       uint8_t *contentRev,
			       uint8_t *formatRev,
			       unsigned short *size)
{
    if (!hdr)
        return FALSE;

    if (contentRev) *contentRev = hdr->ucTableContentRevision;
    if (formatRev) *formatRev = hdr->ucTableFormatRevision;
    if (size) *size = (short)le16_to_cpu(hdr->usStructureSize)
                   - sizeof(ATOM_COMMON_TABLE_HEADER);
    return TRUE;
}

static Bool
rhdAtomAnalyzeMasterDataTable(unsigned char *base,
			      ATOM_MASTER_DATA_TABLE *table,
			      atomDataTablesPtr data)
{
    ATOM_MASTER_LIST_OF_DATA_TABLES *data_table =
        &table->ListOfDataTables;
    unsigned short size;

    if (!rhdAtomAnalyzeCommonHdr(&table->sHeader))
        return FALSE;
    if (!rhdAtomGetTableRevisionAndSize(&table->sHeader,NULL,NULL,
					&size))
        return FALSE;
# define SET_DATA_TABLE(x) {\
   rhdAtomAnalyzeRomDataTable(base,data_table->x,(void *)(&(data->x)),&size); \
    }

# define SET_DATA_TABLE_VERS(x) {\
   rhdAtomAnalyzeRomDataTable(base,data_table->x,&(data->x.base),&size); \
    }

    SET_DATA_TABLE(UtilityPipeLine);
    SET_DATA_TABLE(MultimediaCapabilityInfo);
    SET_DATA_TABLE(MultimediaConfigInfo);
    SET_DATA_TABLE(StandardVESA_Timing);
    SET_DATA_TABLE_VERS(FirmwareInfo);
    SET_DATA_TABLE(DAC_Info);
    SET_DATA_TABLE_VERS(LVDS_Info);
    SET_DATA_TABLE(TMDS_Info);
    SET_DATA_TABLE_VERS(AnalogTV_Info);
    SET_DATA_TABLE_VERS(SupportedDevicesInfo);
    SET_DATA_TABLE(GPIO_I2C_Info);
    SET_DATA_TABLE(VRAM_UsageByFirmware);
    SET_DATA_TABLE(GPIO_Pin_LUT);
    SET_DATA_TABLE(VESA_ToInternalModeLUT);
    SET_DATA_TABLE_VERS(ComponentVideoInfo);
    SET_DATA_TABLE(PowerPlayInfo);
    SET_DATA_TABLE(CompassionateData);
    SET_DATA_TABLE(SaveRestoreInfo);
    SET_DATA_TABLE(PPLL_SS_Info);
    SET_DATA_TABLE(OemInfo);
    SET_DATA_TABLE(XTMDS_Info);
    SET_DATA_TABLE(MclkSS_Info);
    SET_DATA_TABLE(Object_Header);
    SET_DATA_TABLE(IndirectIOAccess);
    SET_DATA_TABLE(MC_InitParameter);
    SET_DATA_TABLE(ASIC_VDDC_Info);
    SET_DATA_TABLE(ASIC_InternalSS_Info);
    SET_DATA_TABLE(TV_VideoMode);
    SET_DATA_TABLE_VERS(VRAM_Info);
    SET_DATA_TABLE(MemoryTrainingInfo);
    SET_DATA_TABLE_VERS(IntegratedSystemInfo);
    SET_DATA_TABLE(ASIC_ProfilingInfo);
    SET_DATA_TABLE(VoltageObjectInfo);
    SET_DATA_TABLE(PowerSourceInfo);
# undef SET_DATA_TABLE

    return TRUE;
}

union atom_supported_devices {
	struct _ATOM_SUPPORTED_DEVICES_INFO info;
	struct _ATOM_SUPPORTED_DEVICES_INFO_2 info_2;
	struct _ATOM_SUPPORTED_DEVICES_INFO_2d1 info_2d1;
};

struct atom_context {
	struct card_info *card;
	void *bios;
	uint32_t cmd_table, data_table;
	uint16_t *iio;

	uint16_t data_block;
	uint32_t fb_base;
	uint32_t divmul[2];
	uint16_t io_attr;
	uint16_t reg_block;
	uint8_t shift;
	int cs_equal, cs_above;
	int io_mode;
	uint32_t *scratch;
    atomDataTables *atomDataPtr;
};

static inline uint8_t get_u8(void *bios, int ptr)
{
    return ((unsigned char *)bios)[ptr];
}
static inline uint16_t get_u16(void *bios, int ptr)
{
    return get_u8(bios ,ptr)|(((uint16_t)get_u8(bios, ptr+1))<<8);
}
#define CU16(ptr) get_u16(ctx->bios, (ptr))
#define CU8(ptr) get_u8(ctx->bios, (ptr))
#define u16 uint16_t
#define u8 uint8_t

Bool atom_parse_data_header(struct atom_context *ctx, int index,
			    uint16_t * size, uint8_t * frev, uint8_t * crev,
			    uint16_t * data_start)
{
	int offset = index * 2 + 4;
	int idx = CU16(ctx->data_table + offset);
	u16 *mdt = (u16 *)(ctx->bios + ctx->data_table + 4);

	if (!mdt[index])
		return FALSE;

	if (size)
		*size = CU16(idx);
	if (frev)
		*frev = CU8(idx + 2);
	if (crev)
		*crev = CU8(idx + 3);
	*data_start = idx;
	return TRUE;
}

#include "radeon.h"

static Bool
show_multimedia(struct atom_context *ctx,
		unsigned char *base, ATOM_ROM_HEADER *atom_rom_hdr)
{
    uint8_t crev, frev;
    unsigned short size;
    int index = GetIndexIntoMasterTable(DATA, MultimediaConfigInfo);
    ATOM_MULTIMEDIA_CONFIG_INFO *mm;
    uint16_t data_offset;
    const char *mm_magic = "$MMT";

    if (!atom_parse_data_header(ctx, index, &size, &frev, &crev,
				&data_offset)) {
	return FALSE;
    }
    mm = (ATOM_MULTIMEDIA_CONFIG_INFO *)(ctx->bios+data_offset);
    if (memcmp(&mm->ulSignature, mm_magic, 4) != 0) {
	return FALSE;
    }
//    printf("Audio chip info: %x\n", mm->ucAudioChipInfo);
    return TRUE;
}


static inline struct radeon_i2c_bus_rec radeon_lookup_i2c_gpio(struct atom_context *ctx,
							       uint8_t id)
{
	ATOM_GPIO_I2C_ASSIGMENT *gpio;
	struct radeon_i2c_bus_rec i2c;
	int index = GetIndexIntoMasterTable(DATA, GPIO_I2C_Info);
	struct _ATOM_GPIO_I2C_INFO *i2c_info;
	uint16_t data_offset, size;
	int i, num_indices;

	memset(&i2c, 0, sizeof(struct radeon_i2c_bus_rec));
	i2c.valid = FALSE;

	if (atom_parse_data_header(ctx, index, &size, NULL, NULL, &data_offset)) {
		i2c_info = (struct _ATOM_GPIO_I2C_INFO *)(ctx->bios + data_offset);

		num_indices = (size - sizeof(ATOM_COMMON_TABLE_HEADER)) /
			sizeof(ATOM_GPIO_I2C_ASSIGMENT);

		for (i = 0; i < num_indices; i++) {
			gpio = &i2c_info->asGPIO_Info[i];

			/* some evergreen boards have bad data for this entry */
#ifdef notdef
			if (ASIC_IS_DCE4(rdev)) {
				if ((i == 7) &&
				    (gpio->usClkMaskRegisterIndex == 0x1936) &&
				    (gpio->sucI2cId.ucAccess == 0)) {
					gpio->sucI2cId.ucAccess = 0x97;
					gpio->ucDataMaskShift = 8;
					gpio->ucDataEnShift = 8;
					gpio->ucDataY_Shift = 8;
					gpio->ucDataA_Shift = 8;
				}
			}

			/* some DCE3 boards have bad data for this entry */
			if (ASIC_IS_DCE3(rdev)) {
				if ((i == 4) &&
				    (gpio->usClkMaskRegisterIndex == 0x1fda) &&
				    (gpio->sucI2cId.ucAccess == 0x94))
					gpio->sucI2cId.ucAccess = 0x14;
			}
#endif

			if (gpio->sucI2cId.ucAccess == id) {
				i2c.mask_clk_reg = le16_to_cpu(gpio->usClkMaskRegisterIndex) * 4;
				i2c.mask_data_reg = le16_to_cpu(gpio->usDataMaskRegisterIndex) * 4;
				i2c.en_clk_reg = le16_to_cpu(gpio->usClkEnRegisterIndex) * 4;
				i2c.en_data_reg = le16_to_cpu(gpio->usDataEnRegisterIndex) * 4;
				i2c.y_clk_reg = le16_to_cpu(gpio->usClkY_RegisterIndex) * 4;
				i2c.y_data_reg = le16_to_cpu(gpio->usDataY_RegisterIndex) * 4;
				i2c.a_clk_reg = le16_to_cpu(gpio->usClkA_RegisterIndex) * 4;
				i2c.a_data_reg = le16_to_cpu(gpio->usDataA_RegisterIndex) * 4;
				i2c.mask_clk_mask = (1 << gpio->ucClkMaskShift);
				i2c.mask_data_mask = (1 << gpio->ucDataMaskShift);
				i2c.en_clk_mask = (1 << gpio->ucClkEnShift);
				i2c.en_data_mask = (1 << gpio->ucDataEnShift);
				i2c.y_clk_mask = (1 << gpio->ucClkY_Shift);
				i2c.y_data_mask = (1 << gpio->ucDataY_Shift);
				i2c.a_clk_mask = (1 << gpio->ucClkA_Shift);
				i2c.a_data_mask = (1 << gpio->ucDataA_Shift);

				if (gpio->sucI2cId.sbfAccess.bfHW_Capable)
					i2c.hw_capable = TRUE;
				else
					i2c.hw_capable = FALSE;

				if (gpio->sucI2cId.ucAccess == 0xa0)
					i2c.mm_i2c = TRUE;
				else
					i2c.mm_i2c = FALSE;

				i2c.i2c_id = gpio->sucI2cId.ucAccess;

				if (i2c.mask_clk_reg)
					i2c.valid = TRUE;
				break;
			}
		}
	}

	return i2c;
}

static inline struct radeon_i2c_bus_rec radeon_dump_i2c_gpio(struct atom_context *ctx)
{
	ATOM_GPIO_I2C_ASSIGMENT *gpio;
	struct radeon_i2c_bus_rec i2c;
	int index = GetIndexIntoMasterTable(DATA, GPIO_I2C_Info);
	struct _ATOM_GPIO_I2C_INFO *i2c_info;
	uint16_t data_offset, size;
	int i, num_indices;
			uint8_t id;

	memset(&i2c, 0, sizeof(struct radeon_i2c_bus_rec));
	i2c.valid = FALSE;

	if (atom_parse_data_header(ctx, index, &size, NULL, NULL, &data_offset)) {
		i2c_info = (struct _ATOM_GPIO_I2C_INFO *)(ctx->bios + data_offset);

		num_indices = (size - sizeof(ATOM_COMMON_TABLE_HEADER)) /
			sizeof(ATOM_GPIO_I2C_ASSIGMENT);

		for (i = 0; i < num_indices; i++) {
			gpio = &i2c_info->asGPIO_Info[i];

			/* some evergreen boards have bad data for this entry */

			id = gpio->sucI2cId.ucAccess;
			i2c.mask_clk_reg = le16_to_cpu(gpio->usClkMaskRegisterIndex) * 4;
			i2c.mask_data_reg = le16_to_cpu(gpio->usDataMaskRegisterIndex) * 4;
			i2c.en_clk_reg = le16_to_cpu(gpio->usClkEnRegisterIndex) * 4;
			i2c.en_data_reg = le16_to_cpu(gpio->usDataEnRegisterIndex) * 4;
			i2c.y_clk_reg = le16_to_cpu(gpio->usClkY_RegisterIndex) * 4;
			i2c.y_data_reg = le16_to_cpu(gpio->usDataY_RegisterIndex) * 4;
			i2c.a_clk_reg = le16_to_cpu(gpio->usClkA_RegisterIndex) * 4;
			i2c.a_data_reg = le16_to_cpu(gpio->usDataA_RegisterIndex) * 4;
			i2c.mask_clk_mask = (1 << gpio->ucClkMaskShift);
			i2c.mask_data_mask = (1 << gpio->ucDataMaskShift);
			i2c.en_clk_mask = (1 << gpio->ucClkEnShift);
			i2c.en_data_mask = (1 << gpio->ucDataEnShift);
			i2c.y_clk_mask = (1 << gpio->ucClkY_Shift);
			i2c.y_data_mask = (1 << gpio->ucDataY_Shift);
			i2c.a_clk_mask = (1 << gpio->ucClkA_Shift);
			i2c.a_data_mask = (1 << gpio->ucDataA_Shift);

			if (gpio->sucI2cId.sbfAccess.bfHW_Capable)
			    i2c.hw_capable = TRUE;
			else
			    i2c.hw_capable = FALSE;

			if (gpio->sucI2cId.ucAccess == 0xa0)
			    i2c.mm_i2c = TRUE;
			else
			    i2c.mm_i2c = FALSE;

			i2c.i2c_id = gpio->sucI2cId.ucAccess;

			if (i2c.mask_clk_reg)
			    i2c.valid = TRUE;
			printf("I2C: index: %d id: %x\n", i, i2c.i2c_id);
		}
	}

	return i2c;
}

#ifdef notdef
void
radeon_add_atom_encoder(struct atom_context *ctx, uint32_t encoder_enum, uint32_t supported_device)
{
	struct radeon_device *rdev = dev->dev_private;
	struct drm_encoder *encoder;
	struct radeon_encoder *radeon_encoder;

	/* see if we already added it */
	list_for_each_entry(encoder, &dev->mode_config.encoder_list, head) {
		radeon_encoder = to_radeon_encoder(encoder);
		if (radeon_encoder->encoder_enum == encoder_enum) {
			radeon_encoder->devices |= supported_device;
			return;
		}

	}

	/* add a new one */
	radeon_encoder = kzalloc(sizeof(struct radeon_encoder), GFP_KERNEL);
	if (!radeon_encoder)
		return;

	encoder = &radeon_encoder->base;
	switch (rdev->num_crtc) {
	case 1:
		encoder->possible_crtcs = 0x1;
		break;
	case 2:
	default:
		encoder->possible_crtcs = 0x3;
		break;
	case 6:
		encoder->possible_crtcs = 0x3f;
		break;
	}

	radeon_encoder->enc_priv = NULL;

	radeon_encoder->encoder_enum = encoder_enum;
	radeon_encoder->encoder_id = (encoder_enum & OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
	radeon_encoder->devices = supported_device;
	radeon_encoder->rmx_type = RMX_OFF;
	radeon_encoder->underscan_type = UNDERSCAN_OFF;
	radeon_encoder->is_ext_encoder = false;

	switch (radeon_encoder->encoder_id) {
	case ENCODER_OBJECT_ID_INTERNAL_LVDS:
	case ENCODER_OBJECT_ID_INTERNAL_TMDS1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_TMDS1:
	case ENCODER_OBJECT_ID_INTERNAL_LVTM1:
		if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT)) {
			radeon_encoder->rmx_type = RMX_FULL;
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_LVDS);
			radeon_encoder->enc_priv = radeon_atombios_get_lvds_info(radeon_encoder);
		} else {
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_TMDS);
			radeon_encoder->enc_priv = radeon_atombios_set_dig_info(radeon_encoder);
			if (ASIC_IS_AVIVO(rdev))
				radeon_encoder->underscan_type = UNDERSCAN_AUTO;
		}
		drm_encoder_helper_add(encoder, &radeon_atom_dig_helper_funcs);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DAC1:
		drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_DAC);
		radeon_encoder->enc_priv = radeon_atombios_set_dac_info(radeon_encoder);
		drm_encoder_helper_add(encoder, &radeon_atom_dac_helper_funcs);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DAC2:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DAC2:
		drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_TVDAC);
		radeon_encoder->enc_priv = radeon_atombios_set_dac_info(radeon_encoder);
		drm_encoder_helper_add(encoder, &radeon_atom_dac_helper_funcs);
		break;
	case ENCODER_OBJECT_ID_INTERNAL_DVO1:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_DVO1:
	case ENCODER_OBJECT_ID_INTERNAL_DDI:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY:
	case ENCODER_OBJECT_ID_INTERNAL_KLDSCP_LVTMA:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY1:
	case ENCODER_OBJECT_ID_INTERNAL_UNIPHY2:
		if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT)) {
			radeon_encoder->rmx_type = RMX_FULL;
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_LVDS);
			radeon_encoder->enc_priv = radeon_atombios_get_lvds_info(radeon_encoder);
		} else if (radeon_encoder->devices & (ATOM_DEVICE_CRT_SUPPORT)) {
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_DAC);
			radeon_encoder->enc_priv = radeon_atombios_set_dig_info(radeon_encoder);
		} else {
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_TMDS);
			radeon_encoder->enc_priv = radeon_atombios_set_dig_info(radeon_encoder);
			if (ASIC_IS_AVIVO(rdev))
				radeon_encoder->underscan_type = UNDERSCAN_AUTO;
		}
		drm_encoder_helper_add(encoder, &radeon_atom_dig_helper_funcs);
		break;
	case ENCODER_OBJECT_ID_SI170B:
	case ENCODER_OBJECT_ID_CH7303:
	case ENCODER_OBJECT_ID_EXTERNAL_SDVOA:
	case ENCODER_OBJECT_ID_EXTERNAL_SDVOB:
	case ENCODER_OBJECT_ID_TITFP513:
	case ENCODER_OBJECT_ID_VT1623:
	case ENCODER_OBJECT_ID_HDMI_SI1930:
		/* these are handled by the primary encoders */
		radeon_encoder->is_ext_encoder = true;
		if (radeon_encoder->devices & (ATOM_DEVICE_LCD_SUPPORT))
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_LVDS);
		else if (radeon_encoder->devices & (ATOM_DEVICE_CRT_SUPPORT))
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_DAC);
		else
			drm_encoder_init(dev, encoder, &radeon_atom_enc_funcs, DRM_MODE_ENCODER_TMDS);
		drm_encoder_helper_add(encoder, &radeon_atom_ext_helper_funcs);
		break;
	}
}
#endif

static const char *encoder_name(uint8_t id)
{
    if (id > 33) {
	return "unknown";
    }
    return encoder_names[id];
}

static int biosoffset(struct atom_context *ctx, void *ptr)
{
    return ptr - ctx->bios;
/*
    return ctx->bios + data_offset;
    return 0;
*/
}

static Bool
show_connectors_from_obj(struct atom_context *ctx,
			 unsigned char *base, ATOM_ROM_HEADER *atom_rom_hdr)
{
    uint8_t crev, frev;
    unsigned short size;
    int index = GetIndexIntoMasterTable(DATA, Object_Header);
    uint16_t data_offset;

	ATOM_CONNECTOR_OBJECT_TABLE *con_obj;
	ATOM_OBJECT_TABLE *router_obj;
	ATOM_DISPLAY_OBJECT_PATH_TABLE *path_obj;
	ATOM_OBJECT_HEADER *obj_header;
	int i, j, k, path_size, device_support;
	int connector_type;
	u16 igp_lane_info, connector_object_id;
	struct radeon_router router;
	struct radeon_hpd hpd;
	struct radeon_i2c_bus_rec ddc_bus;


    if (!atom_parse_data_header(ctx, index, &size, &frev, &crev,
				&data_offset)) {
	return FALSE;
    }
    if (crev < 2) {
	return FALSE;
    }

	obj_header = (ATOM_OBJECT_HEADER *) (ctx->bios + data_offset);
	path_obj = (ATOM_DISPLAY_OBJECT_PATH_TABLE *)
	    (ctx->bios + data_offset +
	     le16_to_cpu(obj_header->usDisplayPathTableOffset));
	con_obj = (ATOM_CONNECTOR_OBJECT_TABLE *)
	    (ctx->bios + data_offset +
	     le16_to_cpu(obj_header->usConnectorObjectTableOffset));
	router_obj = (ATOM_OBJECT_TABLE *)
		(ctx->bios + data_offset +
		 le16_to_cpu(obj_header->usRouterObjectTableOffset));
	device_support = le16_to_cpu(obj_header->usDeviceSupport);

	path_size = 0;
	for (i = 0; i < path_obj->ucNumOfDispPath; i++) {
		uint8_t *addr = (uint8_t *) path_obj->asDispPath;
		ATOM_DISPLAY_OBJECT_PATH *path;
		addr += path_size;
		path = (ATOM_DISPLAY_OBJECT_PATH *) addr;
		path_size += le16_to_cpu(path->usSize);

		if (device_support & le16_to_cpu(path->usDeviceTag)) {
			uint8_t con_obj_id, con_obj_num, con_obj_type;

			con_obj_id =
			    (le16_to_cpu(path->usConnObjectId) & OBJECT_ID_MASK)
			    >> OBJECT_ID_SHIFT;
			con_obj_num =
			    (le16_to_cpu(path->usConnObjectId) & ENUM_ID_MASK)
			    >> ENUM_ID_SHIFT;
			con_obj_type =
			    (le16_to_cpu(path->usConnObjectId) &
			     OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;

			/* TODO CV support */
			if (le16_to_cpu(path->usDeviceTag) ==
				ATOM_DEVICE_CV_SUPPORT)
				continue;

			igp_lane_info = 0;
			connector_type =
			    object_connector_convert[con_obj_id];
			printf("Connector at index %d\n\tType [@offset %d]: %s (%d)\n", i,
			       biosoffset(ctx, &path->usConnObjectId),
			       connectortype_to_name(connector_type), connector_type);
			connector_object_id = con_obj_id;

			if (connector_type == DRM_MODE_CONNECTOR_Unknown)
				continue;
			router.ddc_valid = FALSE;
			router.cd_valid = FALSE;
			for (j = 0; j < ((le16_to_cpu(path->usSize) - 8) / 2); j++) {
				uint8_t grph_obj_id, grph_obj_num, grph_obj_type;

				grph_obj_id =
				    (le16_to_cpu(path->usGraphicObjIds[j]) &
				     OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
				grph_obj_num =
				    (le16_to_cpu(path->usGraphicObjIds[j]) &
				     ENUM_ID_MASK) >> ENUM_ID_SHIFT;
				grph_obj_type =
				    (le16_to_cpu(path->usGraphicObjIds[j]) &
				     OBJECT_TYPE_MASK) >> OBJECT_TYPE_SHIFT;

				if (grph_obj_type == GRAPH_OBJECT_TYPE_ENCODER) {
				    uint16_t id;
					u16 encoder_obj = le16_to_cpu(path->usGraphicObjIds[j]);

					id = (encoder_obj & OBJECT_ID_MASK) >> OBJECT_ID_SHIFT;
					//printf("\tEncoder enum: %x\n", encoder_obj);
				    printf("\tEncoder [@offset %d]: %s (0x%x, 0x)\n", biosoffset(ctx, &path->usGraphicObjIds[j]), encoder_name(grph_obj_id), grph_obj_id);
				} else if (grph_obj_type == GRAPH_OBJECT_TYPE_ROUTER) {
					printf("ROOOOUTER!!!");
					for (k = 0; k < router_obj->ucNumberOfObjects; k++) {
						u16 router_obj_id = le16_to_cpu(router_obj->asObjects[k].usObjectID);
						if (le16_to_cpu(path->usGraphicObjIds[j]) == router_obj_id) {
							ATOM_COMMON_RECORD_HEADER *record = (ATOM_COMMON_RECORD_HEADER *)
								(ctx->bios + data_offset +
								 le16_to_cpu(router_obj->asObjects[k].usRecordOffset));
							ATOM_I2C_RECORD *i2c_record;
							ATOM_I2C_ID_CONFIG_ACCESS *i2c_config;
							ATOM_ROUTER_DDC_PATH_SELECT_RECORD *ddc_path;
							ATOM_ROUTER_DATA_CLOCK_PATH_SELECT_RECORD *cd_path;
							ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT *router_src_dst_table =
								(ATOM_SRC_DST_TABLE_FOR_ONE_OBJECT *)
								(ctx->bios + data_offset +
								 le16_to_cpu(router_obj->asObjects[k].usSrcDstTableOffset));
							int enum_id;

							router.router_id = router_obj_id;
							for (enum_id = 0; enum_id < router_src_dst_table->ucNumberOfDst;
							     enum_id++) {
								if (le16_to_cpu(path->usConnObjectId) ==
								    le16_to_cpu(router_src_dst_table->usDstObjectID[enum_id]))
									break;
							}

							while (record->ucRecordType > 0 &&
							       record->ucRecordType <= ATOM_MAX_OBJECT_RECORD_NUMBER) {
								switch (record->ucRecordType) {
								case ATOM_I2C_RECORD_TYPE:
									i2c_record =
										(ATOM_I2C_RECORD *)
										record;
									i2c_config =
										(ATOM_I2C_ID_CONFIG_ACCESS *)
										&i2c_record->sucI2cId;
									router.i2c_info =
									    radeon_lookup_i2c_gpio(ctx,
												       i2c_config->
												       ucAccess);
									router.i2c_addr = i2c_record->ucI2CAddr >> 1;
									printf("i2c: 0x%x", router.i2c_info);
									break;
								case ATOM_ROUTER_DDC_PATH_SELECT_RECORD_TYPE:
									ddc_path = (ATOM_ROUTER_DDC_PATH_SELECT_RECORD *)
										record;
									router.ddc_valid = TRUE;
									router.ddc_mux_type = ddc_path->ucMuxType;
									router.ddc_mux_control_pin = ddc_path->ucMuxControlPin;
									router.ddc_mux_state = ddc_path->ucMuxState[enum_id];
									printf("ddc: 0x%x", router.ddc_mux_control_pin);
									break;
								case ATOM_ROUTER_DATA_CLOCK_PATH_SELECT_RECORD_TYPE:
									cd_path = (ATOM_ROUTER_DATA_CLOCK_PATH_SELECT_RECORD *)
										record;
									router.cd_valid = TRUE;
									router.cd_mux_type = cd_path->ucMuxType;
									router.cd_mux_control_pin = cd_path->ucMuxControlPin;
									router.cd_mux_state = cd_path->ucMuxState[enum_id];
									printf("cd: 0x%x", router.cd_mux_control_pin);
									break;
								}
								record = (ATOM_COMMON_RECORD_HEADER *)
									((char *)record + record->ucRecordSize);
							}
						}
					}
				}
			}

#ifdef notdef
			/* look up gpio for ddc, hpd */
			ddc_bus.valid = false;
#endif
			hpd.hpd = RADEON_HPD_NONE;
			if ((le16_to_cpu(path->usDeviceTag) &
			     (ATOM_DEVICE_TV_SUPPORT | ATOM_DEVICE_CV_SUPPORT)) == 0) {
				for (j = 0; j < con_obj->ucNumberOfObjects; j++) {
					if (le16_to_cpu(path->usConnObjectId) ==
					    le16_to_cpu(con_obj->asObjects[j].
							usObjectID)) {
						ATOM_COMMON_RECORD_HEADER
						    *record =
						    (ATOM_COMMON_RECORD_HEADER
						     *)
						    (ctx->bios + data_offset +
						     le16_to_cpu(con_obj->
								 asObjects[j].
								 usRecordOffset));
						ATOM_I2C_RECORD *i2c_record;
						ATOM_HPD_INT_RECORD *hpd_record;
						ATOM_I2C_ID_CONFIG_ACCESS *i2c_config;
						
						ATOM_ROUTER_DDC_PATH_SELECT_RECORD *test;

						while (record->ucRecordType > 0
						       && record->
						       ucRecordType <=
						       ATOM_MAX_OBJECT_RECORD_NUMBER) {
							switch (record->ucRecordType) {
							case ATOM_I2C_RECORD_TYPE:
								i2c_record =
								    (ATOM_I2C_RECORD *)
									record;
								i2c_config =
									(ATOM_I2C_ID_CONFIG_ACCESS *)
									&i2c_record->sucI2cId;
								printf("\ti2cid [@offset %d]: 0x%x, OSX senseid: 0x%x\n", biosoffset(ctx, &i2c_config->ucAccess),
								       i2c_config->ucAccess, (i2c_config->ucAccess & 0xf)+1);
								ddc_bus = radeon_lookup_i2c_gpio(ctx,
												 i2c_config->
												 ucAccess);
								break;
							case ATOM_HPD_INT_RECORD_TYPE:
								hpd_record =
									(ATOM_HPD_INT_RECORD *)
									record;
#ifdef notdef
								gpio = radeon_lookup_gpio(ctx,
											  hpd_record->ucHPDIntGPIOID);
								hpd = radeon_atom_get_hpd_info_from_gpio(rdev, &gpio);
#endif
								hpd.plugged_state = hpd_record->ucPlugged_PinState;
								
								printf("\tHotPlugID: %x\n", hpd_record->ucHPDIntGPIOID);

								break;
							case ATOM_ROUTER_DDC_PATH_SELECT_RECORD_TYPE:
								test = (ATOM_ROUTER_DDC_PATH_SELECT_RECORD *) record;
								
								printf("TEST!!! 0x%x", test->ucMuxControlPin);
							}
							record =
							    (ATOM_COMMON_RECORD_HEADER
							     *) ((char *)record
								 +
								 record->
								 ucRecordSize);
						}
						break;
					}
				}
			}

#ifdef notdef
			/* needed for aux chan transactions */
			ddc_bus.hpd = hpd.hpd;

			conn_id = le16_to_cpu(path->usConnObjectId);

			if (!radeon_atom_apply_quirks
			    (dev, le16_to_cpu(path->usDeviceTag), &connector_type,
			     &ddc_bus, &conn_id, &hpd))
				continue;

			radeon_add_atom_connector(dev,
						  conn_id,
						  le16_to_cpu(path->
							      usDeviceTag),
						  connector_type, &ddc_bus,
						  igp_lane_info,
						  connector_object_id,
						  &hpd,
						  &router);

#endif
		}
	}
	return TRUE;
}

static Bool
show_connectors(struct atom_context *ctx,
		unsigned char *base, ATOM_ROM_HEADER *atom_rom_hdr,
		unsigned int data_offset)
{
    int max_device;
    union atom_supported_devices *supported_devices;
    uint16_t device_support;
    int i;
    u_char connectortype;
    uint8_t crev, frev;
    unsigned short size;
#define NEW
#ifdef NEW
    int index = GetIndexIntoMasterTable(DATA, SupportedDevicesInfo);
    uint16_t dataoff;
#endif
    atomDataTables *atomDataPtr;

    atomDataPtr = ctx->atomDataPtr;


#ifdef NEW
    if (!atom_parse_data_header(ctx, index, &size, &frev, &crev,
				&dataoff)) {
	return FALSE;
    }
    data_offset = dataoff;
#else
    if (!rhdAtomGetTableRevisionAndSize(
	    (ATOM_COMMON_TABLE_HEADER *)(&atomDataPtr->SupportedDevicesInfo.SupportedDevicesInfo->sHeader),
	    &crev, &frev, &size)) {
	return FALSE;
    }
    
#endif


    if (frev > 1)
	max_device = ATOM_MAX_SUPPORTED_DEVICE;
    else
	max_device = ATOM_MAX_SUPPORTED_DEVICE_INFO;



    supported_devices =
	(union atom_supported_devices *)(ctx->bios + data_offset);
    device_support = le16_to_cpu(supported_devices->info.usDeviceSupport);
    for (i = 0; i < max_device; i++) {
	ATOM_CONNECTOR_INFO_I2C ci =
	    supported_devices->info.asConnInfo[i];
	if (!(device_support & (1 << i))) {
	    continue;
	}
	connectortype = ci.sucConnectorInfo.sbfAccess.bfConnectorType;
	printf("Connector at index %d type: %s\n", i,
	       connectortype_to_name(connectortype));
    }
    return TRUE;
}


static void
analyze_pci(unsigned char *pcidata)
{
    USHORT maj, minor;

    printf("PCI ID: ");
    maj = *(USHORT *)&pcidata[4];
    minor = *(USHORT *)&pcidata[6];
    printf("%.4x:%.4x\n", le16_to_cpu(maj), le16_to_cpu(minor));
}

static Bool
rhdAtomGetDataTable(struct atom_context *ctx,
		    unsigned char *base,
		    unsigned int *cmd_offset,
		    unsigned int BIOSImageSize)
{
    unsigned int data_offset;
    unsigned int atom_romhdr_off =  le16_to_cpu(*(unsigned short*)
        (base + OFFSET_TO_POINTER_TO_ATOM_ROM_HEADER));
    ATOM_ROM_HEADER *atom_rom_hdr =
        (ATOM_ROM_HEADER *)(base + atom_romhdr_off);
    atomDataTables *atomDataPtr;

    atomDataPtr = ctx->atomDataPtr;
    if (atom_romhdr_off + sizeof(ATOM_ROM_HEADER) > BIOSImageSize) {
	fprintf(stderr,
		"%s: AtomROM header extends beyond BIOS image\n",__func__);
	return FALSE;
    }

    if (memcmp("ATOM",&atom_rom_hdr->uaFirmWareSignature,4)) {
        fprintf(stderr,"%s: No AtomBios signature found\n",
		   __func__);
        return FALSE;
    }
    printf("ATOM BIOS Rom: \n");
    if (!rhdAtomAnalyzeRomHdr(base, atom_rom_hdr, &data_offset, cmd_offset)) {
        fprintf( stderr, "RomHeader invalid\n");
        return FALSE;
    }
    analyze_pci(base + le16_to_cpu(atom_rom_hdr->usPCI_InfoOffset));

    if (data_offset + sizeof (ATOM_MASTER_DATA_TABLE) > BIOSImageSize) {
	fprintf(stderr,"%s: Atom data table outside of BIOS\n",
		   __func__);
    }

    if (*cmd_offset + sizeof (ATOM_MASTER_COMMAND_TABLE) > BIOSImageSize) {
	fprintf(stderr,"%s: Atom command table outside of BIOS\n",
		   __func__);
    }

    if (!rhdAtomAnalyzeMasterDataTable(base, (ATOM_MASTER_DATA_TABLE *)
				       (base + data_offset),
				       atomDataPtr)) {
        fprintf(stderr, "%s: ROM Master Table invalid\n",
		   __func__);
        return FALSE;
    }
    
#define ATOM_ROM_DATA_PTR	0x20

    ctx->data_table = CU16(atom_romhdr_off + ATOM_ROM_DATA_PTR);
    show_multimedia(ctx, base, atom_rom_hdr);

    if (!show_connectors(ctx, base, atom_rom_hdr, data_offset)) {
	show_connectors_from_obj(ctx, base, atom_rom_hdr);
    }
/*    radeon_dump_i2c_gpio(ctx);*/
    return TRUE;
}

#define ROMLEN 65536

int
main(int argc, char **argv)
{
    unsigned char buf[ROMLEN];
    int cnt, f;
    unsigned int offset;
    atomDataTables atomData;
    struct atom_context ctx;
    
    memset(&atomData, 0, sizeof(atomData));
    ctx.bios = buf;
    ctx.atomDataPtr = &atomData;
    f = 0;
    cnt = read(f, buf, ROMLEN);
    if (cnt > 0) {
	rhdAtomGetDataTable(&ctx, buf, &offset, ROMLEN);
    }
    return 0;
}
