using System;
using System.IO;
using System.Linq;
using System.Reflection;
using UnityEditor;
using UnityEngine;

namespace WorldWalker.PlayerGirlAclFixed
{
    /// <summary>
    /// Reuses the validated PlayerGirl FBX baker for ACL-corrected .anim files.
    /// The original 434-clip source folder remains untouched; corrected clips live
    /// in a separate Unity asset directory and FBXs are written to a separate root.
    /// </summary>
    public static class PlayerGirlAclFixedExportBridge
    {
        private const string ModelPath =
            "Assets/DirectPlayerGirl/Avatar_Girl_Sword_PlayerGirl_HeroEntity.fbx";
        private const string CharacterRootName = "Avatar_Girl_Sword_PlayerGirl";
        private const string CorrectedClipRoot = "Assets/PlayerGirlAclFixedSource";

        public static void Export()
        {
            AssetDatabase.Refresh(ImportAssetOptions.ForceSynchronousImport);
            GameObject importedModel = AssetDatabase.LoadAssetAtPath<GameObject>(ModelPath);
            if (importedModel == null)
            {
                throw new InvalidOperationException("Missing PlayerGirl model: " + ModelPath);
            }

            Transform characterRoot = importedModel.transform.Find(CharacterRootName);
            GameObject modelAsset = characterRoot != null
                ? characterRoot.gameObject
                : importedModel;
            AnimationClip[] clips = AssetDatabase.FindAssets(
                    "t:AnimationClip", new[] { CorrectedClipRoot })
                .Select(AssetDatabase.GUIDToAssetPath)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .Select(AssetDatabase.LoadAssetAtPath<AnimationClip>)
                .Where(clip => clip != null)
                .OrderBy(clip => clip.name, StringComparer.OrdinalIgnoreCase)
                .ToArray();
            if (clips.Length == 0)
            {
                throw new InvalidOperationException(
                    "No ACL-corrected clips under " + CorrectedClipRoot);
            }

            string outputDirectory = Environment.GetEnvironmentVariable(
                "WW_PLAYERGIRL_ACL_FIXED_FBX_OUTPUT");
            if (string.IsNullOrWhiteSpace(outputDirectory))
            {
                outputDirectory = Path.GetFullPath(Path.Combine(
                    Application.dataPath, "../PlayerGirlAclFixedFBX"));
            }
            Directory.CreateDirectory(outputDirectory);

            Type exporterType = Type.GetType(
                "PlayerGirlOriginalExport.PlayerGirlOriginalFbxExporter, Assembly-CSharp-Editor");
            if (exporterType == null)
            {
                throw new InvalidOperationException(
                    "PlayerGirlOriginalFbxExporter is not available.");
            }
            const BindingFlags Flags = BindingFlags.Static | BindingFlags.NonPublic;
            MethodInfo ensureTempFolder = exporterType.GetMethod(
                "EnsureTempFolder", Flags);
            MethodInfo validateCompatibility = exporterType.GetMethod(
                "ValidateCompatibility", Flags);
            MethodInfo exportClip = exporterType.GetMethod("ExportClip", Flags);
            if (ensureTempFolder == null || validateCompatibility == null || exportClip == null)
            {
                throw new MissingMethodException(
                    "PlayerGirl FBX baker private API changed.");
            }

            ensureTempFolder.Invoke(null, null);
            foreach (AnimationClip clip in clips)
            {
                validateCompatibility.Invoke(null, new object[] { modelAsset, clip });
                exportClip.Invoke(null, new object[] { modelAsset, clip, outputDirectory });
                Debug.LogFormat(
                    "PLAYERGIRL_ACL_FIXED_FBX_EXPORTED name={0} length={1:F3}",
                    clip.name,
                    clip.length);
            }
            Debug.LogFormat(
                "PLAYERGIRL_ACL_FIXED_FBX_EXPORT_COMPLETE count={0} output={1}",
                clips.Length,
                outputDirectory);
        }
    }
}
