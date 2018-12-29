//
// Copyright 2006 The Android Open Source Project
//
// Android Asset Packaging Tool main entry point.
//
#include "AaptXml.h"
//#include "ApkBuilder.h"
//#include "Images.h"
//#include "Main.h"
//#include "ResourceFilter.h"
//#include "ResourceTable.h"
#include "XMLNode.h"

//#include <utils/Errors.h>
//#include <utils/KeyedVector.h>
//#include <utils/List.h>
//#include <utils/Log.h>
//#include <utils/SortedVector.h>
//#include <utils/threads.h>
//#include <utils/Vector.h>

//#include <errno.h>
//#include <fcntl.h>

//#include <iostream>
//#include <string>
//#include <sstream>

//using namespace android;

/*
 * Return the percent reduction in size (0% == no compression).
 */
int calcPercent(long uncompressedLen, long compressedLen)
{
    if (!uncompressedLen) {
        return 0;
    } else {
        return (int) (100.0 - (compressedLen * 100.0) / uncompressedLen + 0.5);
    }
}

// These are attribute resource constants for the platform, as found
// in android.R.attr
enum {
    LABEL_ATTR = 0x01010001,
};

int main(int argc, char **argv)
{
    if (argc < 2) return 1;


    status_t result = UNKNOWN_ERROR;

    const char* filename = argv[1];

    AssetManager assets;
    int32_t assetsCookie;

    if (!assets.addAssetPath(String8(filename), &assetsCookie)) {
        fprintf(stderr, "ERROR: dump failed because assets could not be loaded\n");
        return 1;
    }

    // Make a dummy config for retrieving resources...  we need to supply
    // non-default values for some configs so that we can retrieve resources
    // in the app that don't have a default.  The most important of these is
    // the API version because key resources like icons will have an implicit
    // version if they are using newer config types like density.
    ResTable_config config;
    memset(&config, 0, sizeof(ResTable_config));
    config.language[0] = 'e';
    config.language[1] = 'n';
    config.country[0] = 'U';
    config.country[1] = 'S';
    config.orientation = ResTable_config::ORIENTATION_PORT;
    config.density = ResTable_config::DENSITY_MEDIUM;
    config.sdkVersion = 10000; // Very high.
    config.screenWidthDp = 320;
    config.screenHeightDp = 480;
    config.smallestScreenWidthDp = 320;
    config.screenLayout |= ResTable_config::SCREENSIZE_NORMAL;
    assets.setConfiguration(config);

    const ResTable& res = assets.getResources(false);
    if (res.getError() != NO_ERROR) {
        fprintf(stderr, "ERROR: dump failed because the resource table is invalid/corrupt.\n");
        return 1;
    }

    // Source for AndroidManifest.xml
    const String8 manifestFile("AndroidManifest.xml");

    // The dynamicRefTable can be null if there are no resources for this asset cookie.
    // This fine.
    const DynamicRefTable* dynamicRefTable = res.getDynamicRefTableForCookie(assetsCookie);

    Asset* asset = NULL;

        asset = assets.openNonAsset(assetsCookie, "AndroidManifest.xml", Asset::ACCESS_BUFFER);
        if (asset == NULL) {
            fprintf(stderr, "ERROR: dump failed because no AndroidManifest.xml found\n");
            //got bail;
        }

        ResXMLTree tree(dynamicRefTable);
        if (tree.setTo(asset->getBuffer(true),
                       asset->getLength()) != NO_ERROR) {
            fprintf(stderr, "ERROR: AndroidManifest.xml is corrupt\n");
            //got bail;
        }
        tree.restart();

        
            Vector<String8> locales;
            res.getLocales(&locales);

            Vector<ResTable_config> configs;
            res.getConfigurations(&configs);

            size_t len;
            ResXMLTree::event_code_t code;
            int depth = 0;
            String8 error;
            String8 pkg;

            String8 packageLabel;
            String8 packageName;

            while ((code=tree.next()) != ResXMLTree::END_DOCUMENT &&
                    code != ResXMLTree::BAD_DOCUMENT) {
                if (code == ResXMLTree::END_TAG) depth--;
                if (code != ResXMLTree::START_TAG) continue;
                depth++;

                const char16_t* ctag16 = tree.getElementName(&len);
                if (ctag16 == NULL) {
                    SourcePos(manifestFile, tree.getLineNumber()).error(
                            "ERROR: failed to get XML element name (bad string pool)");
                    //got bail;
                }
                String8 tag(ctag16);
                if (depth >2) {
                    continue;
                } else if (depth == 1) {
                    if (tag != "manifest") {
                        SourcePos(manifestFile, tree.getLineNumber()).error(
                                "ERROR: manifest does not start with <manifest> tag");
                        //got bail;
                    }
                    pkg = AaptXml::getAttribute(tree, NULL, "package", NULL);
                    packageName = ResTable::normalizeForOutput(pkg.string());
                } else if (depth == 2) {
                    if (tag == "application") {

                        String8 label;
                        const size_t NL = locales.size();
                        for (size_t i=0; i<NL; i++) {
                            const char* localeStr =  locales[i].string();
                            assets.setConfiguration(config, localeStr != NULL ? localeStr : "");
                            String8 llabel = AaptXml::getResolvedAttribute(res, tree, LABEL_ATTR,
                                    &error);
                            if (llabel != "") {
                                if (localeStr == NULL || strlen(localeStr) == 0) {
                                    label = llabel;
                                } else {
                                    if (label == "") {
                                        label = llabel;
                                    }
                                }
                            }
                        }

                        assets.setConfiguration(config);
                        packageLabel = ResTable::normalizeForOutput(label.string());
                    }
                }
            }

    printf("package: name = %s; label = %s\n", packageName.string(), packageLabel.string());

    result = NO_ERROR;
    return 0;
}
