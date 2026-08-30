using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEditor;
using UnityEditor.Animations;
using UnityEditor.Formats.Fbx.Exporter;
using UnityEngine;
using UnityEngine.Animations;
using UnityEngine.Playables;

namespace AvatarGirlOriginalExport
{
    public static class AvatarGirlOriginalLocomotionExporter
    {
        private const string ModelPath =
            "Assets/DirectPlayerGirl/Avatar_Girl_Sword_PlayerGirl_HeroEntity.fbx";
        private const string CharacterRootName = "Avatar_Girl_Sword_PlayerGirl";
        private const string MovementSourceRoot = "Assets/AvatarGirlGenericFixedSource";
        private const string SecondaryClipRoot = "Assets/PlayerGirlSource/Animations";
        private const string TempRoot = "Assets/AvatarGirlOriginalLocomotionTemp";
        private const float BakeFrameRate = 60.0f;

        private sealed class ClipSpec
        {
            public readonly string SourceFile;
            public readonly string OutputName;
            public readonly string SecondaryName;
			public readonly bool UseSecondaryUpperBody;

            public ClipSpec(
				string sourceFile,
				string outputName,
				string secondaryName,
				bool useSecondaryUpperBody = false)
            {
                SourceFile = sourceFile;
                OutputName = outputName;
                SecondaryName = secondaryName;
				UseSecondaryUpperBody = useSecondaryUpperBody;
            }
        }

        private readonly struct LocalTransformSnapshot
        {
            public readonly Transform Transform;
            public readonly Vector3 Position;
            public readonly Quaternion Rotation;
            public readonly Vector3 Scale;

            public LocalTransformSnapshot(Transform transform)
            {
                Transform = transform;
                Position = transform.localPosition;
                Rotation = transform.localRotation;
                Scale = transform.localScale;
            }

            public void Restore()
            {
                Transform.localPosition = Position;
                Transform.localRotation = Rotation;
                Transform.localScale = Scale;
            }
        }

        private static readonly ClipSpec[] ClipSpecs =
        {
            new ClipSpec(
                "Ani_Avatar_Girl_Sword_Ayaka_Attack_01.anim",
                "Ani_Avatar_Girl_Attack_01",
                "Ani_Avatar_Girl_Sword_PlayerGirl_Attack_01",
				true),
            new ClipSpec(
                "Ani_Avatar_Girl_Sword_Ayaka_Attack_02.anim",
                "Ani_Avatar_Girl_Attack_02",
                "Ani_Avatar_Girl_Sword_PlayerGirl_Attack_02",
				true),
            new ClipSpec(
                "Ani_Avatar_Girl_Sword_Ayaka_Attack_03.anim",
                "Ani_Avatar_Girl_Attack_03",
                "Ani_Avatar_Girl_Sword_PlayerGirl_Attack_03",
				true),
            new ClipSpec(
                "Ani_Avatar_Girl_Sword_Ayaka_Attack_04.anim",
                "Ani_Avatar_Girl_Attack_04",
                "Ani_Avatar_Girl_Sword_PlayerGirl_Attack_04",
				true),
            new ClipSpec(
                "Ani_Avatar_Girl_Sword_Ayaka_Attack_05.anim",
                "Ani_Avatar_Girl_Attack_05",
                "Ani_Avatar_Girl_Sword_PlayerGirl_Attack_05",
				true),
            new ClipSpec(
                "Ani_Avatar_Girl_Sword_Ayaka_ExtraAttack.anim",
                "Ani_Avatar_Girl_ExtraAttack",
                "Ani_Avatar_Girl_Sword_PlayerGirl_ExtraAttack",
				true),
            new ClipSpec(
                "Ani_Avatar_Girl_FlyNormal.anim",
                "Ani_Avatar_Girl_FallingAttack_Loop",
                "Ani_Avatar_Girl_Sword_PlayerGirl_FallingAttack_Loop"),
            new ClipSpec(
                "Ani_Avatar_Girl_FallToGroundH.anim",
                "Ani_Avatar_Girl_FallingAttack_Strike",
                "Ani_Avatar_Girl_Sword_PlayerGirl_FallingAttack_Strike"),
            new ClipSpec(
                "Ani_Avatar_Girl_Sword_WeaponStandby.anim",
                "Ani_Avatar_Girl_Standby",
                "Ani_Avatar_Girl_Sword_PlayerGirl_Standby"),
            new ClipSpec(
                "Ani_Avatar_Girl_WalkCycle.anim",
                "Ani_Avatar_Girl_WalkCycle",
                "Ani_Avatar_Girl_Sword_PlayerGirl_WalkCycle"),
            new ClipSpec(
                "Ani_Avatar_Girl_RunCycle.anim",
                "Ani_Avatar_Girl_RunCycle",
                "Ani_Avatar_Girl_Sword_PlayerGirl_RunCycle"),
            new ClipSpec(
                "Ani_Avatar_Girl_SprintCycle.anim",
                "Ani_Avatar_Girl_SprintCycle",
                "Ani_Avatar_Girl_Sword_PlayerGirl_SprintCycle"),
            new ClipSpec(
                "Ani_Avatar_Girl_RunBS.anim",
                "Ani_Avatar_Girl_RunBS",
                "Ani_Avatar_Girl_Sword_PlayerGirl_RunBS"),
            new ClipSpec(
                "Ani_Avatar_Girl_SprintBS.anim",
                "Ani_Avatar_Girl_SprintBS",
                "Ani_Avatar_Girl_Sword_PlayerGirl_SprintBS"),
            new ClipSpec(
                "Assets/GenericSwordGithubSource/Air Up.FBX",
                "Ani_Avatar_Girl_Jump",
                "Ani_Avatar_Girl_Sword_PlayerGirl_Jump"),
            new ClipSpec(
                "Assets/GenericSwordGithubSource/Air Down.FBX",
                "Ani_Avatar_Girl_FlyNormal",
                "Ani_Avatar_Girl_Sword_PlayerGirl_FlyNormal"),
            new ClipSpec(
                "Assets/GenericSwordGithubSource/Crouch.FBX",
                "Ani_Avatar_Girl_FallToGroundL",
                "Ani_Avatar_Girl_Sword_PlayerGirl_FallToGroundL"),
            new ClipSpec(
                "Assets/GenericSwordGithubSource/Crouch.FBX",
                "Ani_Avatar_Girl_FallToGroundH",
                "Ani_Avatar_Girl_Sword_PlayerGirl_FallToGroundH"),
            new ClipSpec(
                "Ani_Avatar_Girl_CrouchRoll.anim",
                "Ani_Avatar_Girl_CrouchRoll",
                "Ani_Avatar_Girl_Sword_PlayerGirl_CrouchRoll"),
            new ClipSpec(
                "Ani_Avatar_Girl_WalkStopL.anim",
                "Ani_Avatar_Girl_WalkStopL",
                "Ani_Avatar_Girl_Sword_PlayerGirl_WalkStopL"),
            new ClipSpec(
                "Ani_Avatar_Girl_RunStopL.anim",
                "Ani_Avatar_Girl_RunStopL",
                "Ani_Avatar_Girl_Sword_PlayerGirl_RunStopL"),
            new ClipSpec(
                "Ani_Avatar_Girl_SprintStopL.anim",
                "Ani_Avatar_Girl_SprintStopL",
                "Ani_Avatar_Girl_Sword_PlayerGirl_SprintStopL"),
            new ClipSpec(
                "Ani_Avatar_Girl_Sword_Hit_L.anim",
                "Ani_Avatar_Girl_Hit_L",
                "Ani_Avatar_Girl_Sword_PlayerGirl_Hit_L"),
            new ClipSpec(
                "Ani_Avatar_Girl_Sword_Hit_H.anim",
                "Ani_Avatar_Girl_Hit_H",
                "Ani_Avatar_Girl_Sword_PlayerGirl_Hit_H"),
            new ClipSpec(
                "Ani_Avatar_Girl_Death.anim",
                "Ani_Avatar_Girl_Death",
                "Ani_Avatar_Girl_Sword_PlayerGirl_Death"),
            new ClipSpec(
                "Ani_Avatar_Girl_SwimStandby.anim",
                "Ani_Avatar_Girl_SwimStandby",
                "Ani_Avatar_Girl_Sword_PlayerGirl_SwimStandby"),
            new ClipSpec(
                "Ani_Avatar_Girl_SwimF.anim",
                "Ani_Avatar_Girl_SwimF",
                "Ani_Avatar_Girl_Sword_PlayerGirl_SwimF"),
            new ClipSpec(
                "Ani_Avatar_Girl_ClimbU.anim",
                "Ani_Avatar_Girl_ClimbU",
                "Ani_Avatar_Girl_Sword_PlayerGirl_ClimbU")
        };

        public static void Export()
        {
            string outputDirectory = ResolveOutputDirectory();
            Directory.CreateDirectory(outputDirectory);
            EnsureTempFolder();

            try
            {
                Avatar avatar = ConfigureAndLoadHumanoidAvatar();
                GameObject modelAsset = AssetDatabase.LoadAssetAtPath<GameObject>(ModelPath);
                if (modelAsset == null)
                {
                    throw new InvalidOperationException("Could not load PlayerGirl model: " + ModelPath);
                }

                HashSet<string> requestedClips = new HashSet<string>(
                    (Environment.GetEnvironmentVariable("WW_AVATARGIRL_ORIGINAL_EXPORT_FILTER") ?? string.Empty)
                        .Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries)
                        .Select(value => value.Trim()),
                    StringComparer.OrdinalIgnoreCase);
                ClipSpec[] selectedSpecs = requestedClips.Count == 0
                    ? ClipSpecs
                    : ClipSpecs.Where(spec => requestedClips.Contains(spec.OutputName)
                        || requestedClips.Contains(spec.SourceFile)
                        || requestedClips.Any(value => spec.OutputName.EndsWith(value,
                            StringComparison.OrdinalIgnoreCase))).ToArray();
                if (selectedSpecs.Length == 0)
                {
                    throw new InvalidOperationException(
                        "WW_AVATARGIRL_ORIGINAL_EXPORT_FILTER matched no clips.");
                }
                foreach (ClipSpec spec in selectedSpecs)
                {
                    string sourcePath = spec.SourceFile.StartsWith(
                        "Assets/", StringComparison.OrdinalIgnoreCase)
                        ? spec.SourceFile
                        : MovementSourceRoot + "/" + spec.SourceFile;
                    AnimationClip bodyClip = ConfigureAndLoadHumanoidClip(sourcePath);
                    AnimationClip secondaryClip = LoadClipByName(
                        SecondaryClipRoot,
                        spec.SecondaryName);
                    ExportMergedClip(
                        modelAsset,
                        avatar,
                        bodyClip,
                        secondaryClip,
						spec.UseSecondaryUpperBody,
                        spec.OutputName,
                        outputDirectory);
                }

                Debug.LogFormat(
                    "AVATARGIRL_ORIGINAL_MERGE_COMPLETE count={0} output={1}",
                    selectedSpecs.Length,
                    outputDirectory);
            }
            finally
            {
                AssetDatabase.DeleteAsset(TempRoot);
                AssetDatabase.Refresh();
            }
        }

        private static AnimationClip ConfigureAndLoadHumanoidClip(string assetPath)
        {
            ModelImporter modelImporter = AssetImporter.GetAtPath(assetPath) as ModelImporter;
            if (modelImporter != null &&
                (modelImporter.animationType != ModelImporterAnimationType.Human ||
                 modelImporter.avatarSetup != ModelImporterAvatarSetup.CreateFromThisModel ||
                 !modelImporter.importAnimation))
            {
                modelImporter.animationType = ModelImporterAnimationType.Human;
                modelImporter.avatarSetup = ModelImporterAvatarSetup.CreateFromThisModel;
                modelImporter.importAnimation = true;
                modelImporter.SaveAndReimport();
            }
            AnimationClip clip = modelImporter == null
                ? AssetDatabase.LoadAssetAtPath<AnimationClip>(assetPath)
                : AssetDatabase.LoadAllAssetsAtPath(assetPath)
                    .OfType<AnimationClip>()
                    .FirstOrDefault(candidate => !candidate.name.StartsWith(
                        "__preview__", StringComparison.OrdinalIgnoreCase));
            if (clip == null || !clip.isHumanMotion)
            {
                throw new InvalidOperationException(
                    "Original Avatar_Girl muscle clip is missing or not Humanoid: " + assetPath);
            }
            Debug.LogFormat(
                "AVATARGIRL_ORIGINAL_SOURCE path={0} clip={1} length={2:F3} human={3}",
                assetPath,
                clip.name,
                clip.length,
                clip.isHumanMotion);
            return clip;
        }

        private static Avatar ConfigureAndLoadHumanoidAvatar()
        {
            ModelImporter importer = AssetImporter.GetAtPath(ModelPath) as ModelImporter;
            if (importer == null)
            {
                throw new InvalidOperationException("ModelImporter was not found for " + ModelPath);
            }

            if (importer.animationType != ModelImporterAnimationType.Human ||
                importer.avatarSetup != ModelImporterAvatarSetup.CreateFromThisModel)
            {
                importer.animationType = ModelImporterAnimationType.Human;
                importer.avatarSetup = ModelImporterAvatarSetup.CreateFromThisModel;
                importer.importAnimation = true;
                importer.SaveAndReimport();
            }

            Avatar avatar = AssetDatabase.LoadAllAssetsAtPath(ModelPath)
                .OfType<Avatar>()
                .FirstOrDefault();
            if (avatar == null)
            {
                throw new InvalidOperationException("Unity did not generate an Avatar for " + ModelPath);
            }

            Debug.LogFormat(
                "PLAYERGIRL_HUMANOID_AVATAR name={0} valid={1} human={2}",
                avatar.name,
                avatar.isValid,
                avatar.isHuman);
            if (!avatar.isValid || !avatar.isHuman)
            {
                throw new InvalidOperationException("PlayerGirl humanoid Avatar is invalid.");
            }
            return avatar;
        }

        private static AnimationClip LoadClipByName(string root, string clipName)
        {
            string[] paths = AssetDatabase.FindAssets("t:AnimationClip", new[] { root })
                .Select(AssetDatabase.GUIDToAssetPath)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .ToArray();
            foreach (string path in paths)
            {
                AnimationClip clip = AssetDatabase.LoadAssetAtPath<AnimationClip>(path);
                if (clip != null && clip.name.Equals(clipName, StringComparison.OrdinalIgnoreCase))
                {
                    return clip;
                }
            }
            throw new InvalidOperationException("Missing clip " + clipName + " under " + root);
        }

        private static void ExportMergedClip(
            GameObject modelAsset,
            Avatar avatar,
            AnimationClip bodyClip,
            AnimationClip secondaryClip,
			bool useSecondaryUpperBody,
            string outputName,
            string outputDirectory)
        {
            string safeName = SanitizeFileName(outputName);
            string bodyControllerPath = TempRoot + "/Body_" + safeName + ".controller";
            string bakedControllerPath = TempRoot + "/Baked_" + safeName + ".controller";
            string bakedClipPath = TempRoot + "/Baked_" + safeName + ".anim";
            AssetDatabase.DeleteAsset(bodyControllerPath);
            AssetDatabase.DeleteAsset(bakedControllerPath);
            AssetDatabase.DeleteAsset(bakedClipPath);

            AnimatorController bodyController = CreateController(bodyControllerPath, bodyClip);
            GameObject instance = UnityEngine.Object.Instantiate(modelAsset);
            instance.name = "PlayerGirl_Merged_" + safeName;
            try
            {
                Animator humanoidAnimator = instance.GetComponent<Animator>();
                if (humanoidAnimator == null)
                {
                    humanoidAnimator = instance.AddComponent<Animator>();
                }
                humanoidAnimator.avatar = avatar;
                humanoidAnimator.runtimeAnimatorController = bodyController;
                humanoidAnimator.applyRootMotion = false;
                humanoidAnimator.cullingMode = AnimatorCullingMode.AlwaysAnimate;
                humanoidAnimator.Rebind();
                humanoidAnimator.Update(0.0f);

                AnimationClip bakedClip = BakeMergedTransformAnimation(
                    instance,
                    humanoidAnimator,
                    bodyClip,
                    secondaryClip,
					useSecondaryUpperBody);
                RenameHierarchyAndClipPathsForUnreal(instance, bakedClip);
                AssetDatabase.CreateAsset(bakedClip, bakedClipPath);
                AssetDatabase.SaveAssets();

                UnityEngine.Object.DestroyImmediate(humanoidAnimator);
                Animator genericAnimator = instance.AddComponent<Animator>();
                AnimatorController bakedController = CreateController(bakedControllerPath, bakedClip);
                genericAnimator.runtimeAnimatorController = bakedController;
                genericAnimator.applyRootMotion = false;
                genericAnimator.cullingMode = AnimatorCullingMode.AlwaysAnimate;
                genericAnimator.Rebind();
                genericAnimator.Update(0.0f);
                AddFullSkeletonCarrier(instance);

                string outputPath = Path.Combine(
                    outputDirectory,
                    "PlayerGirl@" + safeName + "_Merged.fbx");
                ExportAndValidate(outputPath, instance);
            }
            finally
            {
                UnityEngine.Object.DestroyImmediate(instance);
                AssetDatabase.DeleteAsset(bodyControllerPath);
                AssetDatabase.DeleteAsset(bakedControllerPath);
                AssetDatabase.DeleteAsset(bakedClipPath);
            }
        }

        private static AnimatorController CreateController(string path, AnimationClip clip)
        {
            AnimatorController controller = AnimatorController.CreateAnimatorControllerAtPath(path);
            AnimatorState state = controller.layers[0].stateMachine.AddState(clip.name);
            state.motion = clip;
            controller.layers[0].stateMachine.defaultState = state;
            EditorUtility.SetDirty(controller);
            AssetDatabase.SaveAssets();
            return controller;
        }

        private static void AddFullSkeletonCarrier(GameObject instance)
        {
            Transform skeletonRoot = instance.GetComponentsInChildren<Transform>(true)
                .FirstOrDefault(transform => transform.name.Equals(
                    "Bip001",
                    StringComparison.Ordinal));
            if (skeletonRoot == null)
            {
                throw new InvalidOperationException("Bip001 skeleton root was not found.");
            }
            Transform[] bones = skeletonRoot.GetComponentsInChildren<Transform>(true);
            GameObject carrier = new GameObject("WW_FullSkeletonCarrier");
            carrier.transform.SetParent(instance.transform, false);
            SkinnedMeshRenderer renderer = carrier.AddComponent<SkinnedMeshRenderer>();

            Vector3[] vertices = new Vector3[bones.Length * 3];
            int[] triangles = new int[vertices.Length];
            BoneWeight[] weights = new BoneWeight[vertices.Length];
            Matrix4x4[] bindPoses = new Matrix4x4[bones.Length];
            const float tinyOffset = 0.0001f;
            for (int boneIndex = 0; boneIndex < bones.Length; ++boneIndex)
            {
                Vector3 center = carrier.transform.InverseTransformPoint(bones[boneIndex].position);
                int vertexIndex = boneIndex * 3;
                vertices[vertexIndex] = center;
                vertices[vertexIndex + 1] = center + new Vector3(tinyOffset, 0.0f, 0.0f);
                vertices[vertexIndex + 2] = center + new Vector3(0.0f, tinyOffset, 0.0f);
                triangles[vertexIndex] = vertexIndex;
                triangles[vertexIndex + 1] = vertexIndex + 1;
                triangles[vertexIndex + 2] = vertexIndex + 2;
                for (int offset = 0; offset < 3; ++offset)
                {
                    weights[vertexIndex + offset] = new BoneWeight
                    {
                        boneIndex0 = boneIndex,
                        weight0 = 1.0f
                    };
                }
                bindPoses[boneIndex] = bones[boneIndex].worldToLocalMatrix
                    * carrier.transform.localToWorldMatrix;
            }

            Mesh mesh = new Mesh { name = "WW_FullSkeletonCarrierMesh" };
            mesh.vertices = vertices;
            mesh.triangles = triangles;
            mesh.boneWeights = weights;
            mesh.bindposes = bindPoses;
            mesh.RecalculateBounds();
            renderer.sharedMesh = mesh;
            renderer.bones = bones;
            renderer.rootBone = skeletonRoot;
            renderer.updateWhenOffscreen = true;
            foreach (SkinnedMeshRenderer otherRenderer in instance
                .GetComponentsInChildren<SkinnedMeshRenderer>(true)
                .Where(candidate => candidate != renderer)
                .ToArray())
            {
                UnityEngine.Object.DestroyImmediate(otherRenderer);
            }
            Debug.LogFormat(
                "PLAYERGIRL_FULL_SKELETON_CARRIER bones={0} vertices={1}",
                bones.Length,
                vertices.Length);
        }

        private static AnimationClip BakeMergedTransformAnimation(
            GameObject instance,
            Animator animator,
            AnimationClip bodyClip,
            AnimationClip secondaryClip,
			bool useSecondaryUpperBody)
        {
            Transform characterRoot = instance.transform.Find(CharacterRootName);
            if (characterRoot == null)
            {
                throw new InvalidOperationException(
                    "Could not find secondary-animation root " + CharacterRootName);
            }

            Transform[] transforms = instance.GetComponentsInChildren<Transform>(true);
            Dictionary<string, Transform> transformsByPath = transforms.ToDictionary(
                transform => AnimationUtility.CalculateTransformPath(transform, instance.transform),
                transform => transform,
                StringComparer.Ordinal);
            Dictionary<string, BakedTransformCurves> curves = transformsByPath.Keys.ToDictionary(
                path => path,
                path => new BakedTransformCurves(),
                StringComparer.Ordinal);

			// The base Avatar_Girl clips contain a complete Humanoid pose. PlayerGirl
			// clips are sparse character layers, so the protected set depends on the
			// action: ordinary clips protect the whole Humanoid body, while sword attacks
			// protect only root/lower-body stability and deliberately accept PlayerGirl's
			// authored spine, arm, hand and weapon-helper curves.
            HashSet<Transform> primaryBodyTransforms = new HashSet<Transform>();
            for (int boneIndex = 0; boneIndex < (int)HumanBodyBones.LastBone; ++boneIndex)
            {
                Transform bodyTransform = animator.GetBoneTransform((HumanBodyBones)boneIndex);
                if (bodyTransform != null)
                {
                    primaryBodyTransforms.Add(bodyTransform);
                }
            }
            Transform bipRoot = transforms.FirstOrDefault(transform => transform.name == "Bip001");
            if (bipRoot != null)
            {
                primaryBodyTransforms.Add(bipRoot);
            }
			HashSet<Transform> protectedSecondaryTransforms;
			if (useSecondaryUpperBody)
			{
				// PlayerGirl attack clips are sparse character layers. Keep the stable
				// Humanoid base for the root and legs, but let the authored spine, arms,
				// hands and weapon helpers override it so the hand actually follows each
				// sword strike. Sampling only non-Humanoid helpers produced a moving sword
				// with generic or static arms; restoring every Humanoid transform produced
				// six visually identical attacks.
				HumanBodyBones[] lowerBodyBones =
				{
					HumanBodyBones.Hips,
					HumanBodyBones.LeftUpperLeg,
					HumanBodyBones.RightUpperLeg,
					HumanBodyBones.LeftLowerLeg,
					HumanBodyBones.RightLowerLeg,
					HumanBodyBones.LeftFoot,
					HumanBodyBones.RightFoot,
					HumanBodyBones.LeftToes,
					HumanBodyBones.RightToes
				};
				protectedSecondaryTransforms = new HashSet<Transform>(
					lowerBodyBones
						.Select(animator.GetBoneTransform)
						.Where(transform => transform != null));
				if (bipRoot != null)
				{
					protectedSecondaryTransforms.Add(bipRoot);
				}
			}
			else
			{
				protectedSecondaryTransforms = primaryBodyTransforms;
			}
            Debug.LogFormat(
                "AVATARGIRL_PRIMARY_BODY_MASK transforms={0} protected={1} mode={2}",
				primaryBodyTransforms.Count,
				protectedSecondaryTransforms.Count,
				useSecondaryUpperBody ? "PlayerGirlAttackUpperBody" : "NonHumanoidOnly");
            bool includeSecondaryLayer = string.Equals(
                Environment.GetEnvironmentVariable("WW_AVATARGIRL_INCLUDE_SECONDARY"),
                "1",
                StringComparison.Ordinal) || useSecondaryUpperBody;
            Debug.LogFormat(
                "AVATARGIRL_SECONDARY_LAYER enabled={0}",
                includeSecondaryLayer);

            string[] diagnosticBoneNames =
            {
                "Bip001 L Thigh", "Bip001 R Thigh",
                "Bip001 L Calf", "Bip001 R Calf",
				"Bip001 L UpperArm", "Bip001 R UpperArm",
				"Bip001 L Forearm", "Bip001 R Forearm",
				"Bip001 L Hand", "Bip001 R Hand",
				"WeaponR"
            };
            Dictionary<string, Transform> diagnosticBones = diagnosticBoneNames.ToDictionary(
                name => name,
                name => transforms.FirstOrDefault(
                    transform => transform.name.Equals(name, StringComparison.Ordinal)),
                StringComparer.Ordinal);
            if (diagnosticBones.Values.Any(transform => transform == null))
            {
                throw new InvalidOperationException(
                    "Missing diagnostic bones: " + string.Join(", ", diagnosticBones
                        .Where(pair => pair.Value == null)
                        .Select(pair => pair.Key)));
            }

            int frameCount = Mathf.Max(1, Mathf.CeilToInt(bodyClip.length * BakeFrameRate));
            Dictionary<string, Quaternion> firstRotations = new Dictionary<string, Quaternion>(
                StringComparer.Ordinal);
            Dictionary<string, float> maximumAngles = diagnosticBoneNames.ToDictionary(
                name => name,
                name => 0.0f,
                StringComparer.Ordinal);

            PlayableGraph graph = PlayableGraph.Create("PlayerGirlHumanoidBake_" + bodyClip.name);
            graph.SetTimeUpdateMode(DirectorUpdateMode.Manual);
            AnimationPlayableOutput output = AnimationPlayableOutput.Create(
                graph,
                "PlayerGirlHumanoidOutput",
                animator);
            AnimationClipPlayable bodyPlayable = AnimationClipPlayable.Create(graph, bodyClip);
            bodyPlayable.SetApplyFootIK(false);
            bodyPlayable.SetApplyPlayableIK(false);
            output.SetSourcePlayable(bodyPlayable);
            graph.Play();
            try
            {
                for (int frame = 0; frame <= frameCount; ++frame)
                {
                    float time = Mathf.Min(bodyClip.length, frame / BakeFrameRate);
                    float normalizedTime = bodyClip.length > 0.0f ? time / bodyClip.length : 0.0f;
                    bodyPlayable.SetTime(time);
                    graph.Evaluate(0.0f);

                    if (includeSecondaryLayer)
                    {
						LocalTransformSnapshot[] primaryBodyPose = protectedSecondaryTransforms
                            .Select(transform => new LocalTransformSnapshot(transform))
                            .ToArray();
                        float secondaryTime = secondaryClip.length > 0.0f
                            ? Mathf.Min(secondaryClip.length, normalizedTime * secondaryClip.length)
                            : 0.0f;
                        secondaryClip.SampleAnimation(characterRoot.gameObject, secondaryTime);
                        foreach (LocalTransformSnapshot snapshot in primaryBodyPose)
                        {
                            snapshot.Restore();
                        }
                    }

                    foreach (KeyValuePair<string, Transform> bone in diagnosticBones)
                    {
                        Quaternion rotation = SanitizeQuaternion(bone.Value.localRotation, Quaternion.identity);
                        if (frame == 0)
                        {
                            firstRotations[bone.Key] = rotation;
                        }
                        maximumAngles[bone.Key] = Mathf.Max(
                            maximumAngles[bone.Key],
                            Quaternion.Angle(firstRotations[bone.Key], rotation));
                    }

                    foreach (KeyValuePair<string, Transform> pair in transformsByPath)
                    {
                        curves[pair.Key].AddKey(time, pair.Value);
                    }
                }
            }
            finally
            {
                graph.Destroy();
            }

            float maximumLimbAngle = maximumAngles.Values.Max();
            string angleSummary = string.Join(",", maximumAngles.Select(
                pair => pair.Key + "=" + pair.Value.ToString("F3")));

            Debug.LogFormat(
                "PLAYERGIRL_HUMANOID_BAKED body={0} secondary={1} bodyHuman={2} " +
                "paths={3} frames={4} length={5:F3} maxLimbAngle={6:F3} angles=[{7}]",
                bodyClip.name,
                secondaryClip.name,
                bodyClip.isHumanMotion,
                transformsByPath.Count,
                frameCount + 1,
                bodyClip.length,
                maximumLimbAngle,
                angleSummary);
            if ((bodyClip.name.Contains("WalkCycle") ||
                 bodyClip.name.Contains("RunCycle") ||
                 bodyClip.name.Contains("SprintCycle")) &&
                maximumLimbAngle < 0.5f)
            {
                throw new InvalidOperationException(
                    bodyClip.name + " did not produce meaningful limb motion (" +
                    maximumLimbAngle.ToString("F3") + " degrees).");
            }

            AnimationClip bakedClip = new AnimationClip
            {
                name = bodyClip.name + "_Merged",
                frameRate = BakeFrameRate,
                legacy = false
            };
            foreach (KeyValuePair<string, BakedTransformCurves> pair in curves)
            {
                pair.Value.ApplyTo(bakedClip, pair.Key);
            }
            bakedClip.EnsureQuaternionContinuity();
            AnimationClipSettings sourceSettings = AnimationUtility.GetAnimationClipSettings(bodyClip);
            AnimationClipSettings bakedSettings = AnimationUtility.GetAnimationClipSettings(bakedClip);
            bakedSettings.loopTime = sourceSettings.loopTime;
            bakedSettings.loopBlend = sourceSettings.loopBlend;
            AnimationUtility.SetAnimationClipSettings(bakedClip, bakedSettings);
            return bakedClip;
        }

        private sealed class BakedTransformCurves
        {
            private readonly AnimationCurve px = new AnimationCurve();
            private readonly AnimationCurve py = new AnimationCurve();
            private readonly AnimationCurve pz = new AnimationCurve();
            private readonly AnimationCurve rx = new AnimationCurve();
            private readonly AnimationCurve ry = new AnimationCurve();
            private readonly AnimationCurve rz = new AnimationCurve();
            private readonly AnimationCurve rw = new AnimationCurve();
            private readonly AnimationCurve sx = new AnimationCurve();
            private readonly AnimationCurve sy = new AnimationCurve();
            private readonly AnimationCurve sz = new AnimationCurve();
            private Quaternion? previousRotation;
            private Vector3? previousPosition;
            private Vector3? previousScale;

            public void AddKey(float time, Transform transform)
            {
                Vector3 position = SanitizeVector(
                    transform.localPosition,
                    previousPosition ?? Vector3.zero);
                Quaternion rotation = SanitizeQuaternion(
                    transform.localRotation,
                    previousRotation ?? Quaternion.identity);
                Vector3 scale = SanitizeVector(
                    transform.localScale,
                    previousScale ?? Vector3.one);
                if (previousRotation.HasValue && Quaternion.Dot(previousRotation.Value, rotation) < 0.0f)
                {
                    rotation = new Quaternion(-rotation.x, -rotation.y, -rotation.z, -rotation.w);
                }
                previousRotation = rotation;
                previousPosition = position;
                previousScale = scale;
                px.AddKey(time, position.x);
                py.AddKey(time, position.y);
                pz.AddKey(time, position.z);
                rx.AddKey(time, rotation.x);
                ry.AddKey(time, rotation.y);
                rz.AddKey(time, rotation.z);
                rw.AddKey(time, rotation.w);
                sx.AddKey(time, scale.x);
                sy.AddKey(time, scale.y);
                sz.AddKey(time, scale.z);
            }

            public void ApplyTo(AnimationClip clip, string path)
            {
                SetCurve(clip, path, "m_LocalPosition.x", px);
                SetCurve(clip, path, "m_LocalPosition.y", py);
                SetCurve(clip, path, "m_LocalPosition.z", pz);
                SetCurve(clip, path, "m_LocalRotation.x", rx);
                SetCurve(clip, path, "m_LocalRotation.y", ry);
                SetCurve(clip, path, "m_LocalRotation.z", rz);
                SetCurve(clip, path, "m_LocalRotation.w", rw);
                SetCurve(clip, path, "m_LocalScale.x", sx);
                SetCurve(clip, path, "m_LocalScale.y", sy);
                SetCurve(clip, path, "m_LocalScale.z", sz);
            }

            private static void SetCurve(
                AnimationClip clip,
                string path,
                string property,
                AnimationCurve curve)
            {
                AnimationUtility.SetEditorCurve(
                    clip,
                    EditorCurveBinding.FloatCurve(path, typeof(Transform), property),
                    curve);
            }
        }

        private static void RenameHierarchyAndClipPathsForUnreal(
            GameObject instance,
            AnimationClip clip)
        {
            EditorCurveBinding[] bindings = AnimationUtility.GetCurveBindings(clip);
            List<Tuple<EditorCurveBinding, EditorCurveBinding, AnimationCurve>> remapped =
                new List<Tuple<EditorCurveBinding, EditorCurveBinding, AnimationCurve>>();
            foreach (EditorCurveBinding oldBinding in bindings)
            {
                string newPath = string.Join(
                    "/",
                    oldBinding.path.Split('/').Select(segment => segment.Replace(' ', '-')));
                if (newPath.Equals(oldBinding.path, StringComparison.Ordinal))
                {
                    continue;
                }
                EditorCurveBinding newBinding = oldBinding;
                newBinding.path = newPath;
                remapped.Add(Tuple.Create(
                    oldBinding,
                    newBinding,
                    AnimationUtility.GetEditorCurve(clip, oldBinding)));
            }
            foreach (Tuple<EditorCurveBinding, EditorCurveBinding, AnimationCurve> item in remapped)
            {
                AnimationUtility.SetEditorCurve(clip, item.Item1, null);
            }
            foreach (Tuple<EditorCurveBinding, EditorCurveBinding, AnimationCurve> item in remapped)
            {
                AnimationUtility.SetEditorCurve(clip, item.Item2, item.Item3);
            }

            int renamedCount = 0;
            foreach (Transform transform in instance.GetComponentsInChildren<Transform>(true))
            {
                string renamed = transform.name.Replace(' ', '-');
                if (!renamed.Equals(transform.name, StringComparison.Ordinal))
                {
                    transform.name = renamed;
                    ++renamedCount;
                }
            }
            Debug.LogFormat(
                "PLAYERGIRL_UNREAL_BONE_NAMES renamedTransforms={0} remappedCurves={1}",
                renamedCount,
                remapped.Count);
        }

        private static Vector3 SanitizeVector(Vector3 value, Vector3 fallback)
        {
            return IsFinite(value.x) && IsFinite(value.y) && IsFinite(value.z)
                ? value
                : fallback;
        }

        private static Quaternion SanitizeQuaternion(Quaternion value, Quaternion fallback)
        {
            if (!IsFinite(value.x) || !IsFinite(value.y) ||
                !IsFinite(value.z) || !IsFinite(value.w))
            {
                return fallback;
            }
            float squareMagnitude = value.x * value.x + value.y * value.y +
                value.z * value.z + value.w * value.w;
            if (!IsFinite(squareMagnitude) || squareMagnitude < 1.0e-12f)
            {
                return fallback;
            }
            float inverseMagnitude = 1.0f / Mathf.Sqrt(squareMagnitude);
            return new Quaternion(
                value.x * inverseMagnitude,
                value.y * inverseMagnitude,
                value.z * inverseMagnitude,
                value.w * inverseMagnitude);
        }

        private static bool IsFinite(float value)
        {
            return !float.IsNaN(value) && !float.IsInfinity(value);
        }

        private static void ExportAndValidate(string outputPath, GameObject instance)
        {
            if (File.Exists(outputPath))
            {
                File.Delete(outputPath);
            }
            ExportModelOptions options = new ExportModelOptions
            {
                ExportFormat = ExportFormat.Binary,
                ModelAnimIncludeOption = Include.ModelAndAnim,
                AnimateSkinnedMesh = true
            };
            string result = ModelExporter.ExportObject(outputPath, instance, options);
            if (string.IsNullOrEmpty(result) || !File.Exists(outputPath))
            {
                throw new InvalidOperationException("FBX Exporter did not create " + outputPath);
            }
            if (new FileInfo(outputPath).Length < 1024)
            {
                throw new InvalidOperationException("FBX is unexpectedly small: " + outputPath);
            }
            RewriteFbxCoreBoneNames(outputPath, instance);
        }

        private static void RewriteFbxCoreBoneNames(string outputPath, GameObject instance)
        {
            string temporaryPath = Path.Combine(
                Path.GetDirectoryName(outputPath),
                Path.GetFileNameWithoutExtension(outputPath) + ".renaming.fbx");
            if (File.Exists(temporaryPath))
            {
                File.Delete(temporaryPath);
            }

            Dictionary<string, string> exportedToDesiredName = instance
                .GetComponentsInChildren<Transform>(true)
                .Where(transform => transform.name.Contains("-"))
                .GroupBy(transform => transform.name.Replace('-', '_'), StringComparer.Ordinal)
                .ToDictionary(
                    group => group.Key,
                    group => group.First().name,
                    StringComparer.Ordinal);
            int renamedCount;
            using (Autodesk.Fbx.FbxManager manager = Autodesk.Fbx.FbxManager.Create())
            {
                Autodesk.Fbx.FbxIOSettings settings = Autodesk.Fbx.FbxIOSettings.Create(
                    manager,
                    Autodesk.Fbx.Globals.IOSROOT);
                manager.SetIOSettings(settings);
                Autodesk.Fbx.FbxScene scene = Autodesk.Fbx.FbxScene.Create(
                    manager,
                    "PlayerGirlRenameScene");
                using (Autodesk.Fbx.FbxImporter importer = Autodesk.Fbx.FbxImporter.Create(
                    manager,
                    "PlayerGirlRenameImporter"))
                {
                    if (!importer.Initialize(outputPath, -1, manager.GetIOSettings()) ||
                        !importer.Import(scene))
                    {
                        throw new InvalidOperationException(
                            "FBX SDK could not reopen exported file: " + outputPath);
                    }
                }

                renamedCount = RewriteFbxNodeNames(
                    scene.GetRootNode(),
                    exportedToDesiredName);
                using (Autodesk.Fbx.FbxExporter exporter = Autodesk.Fbx.FbxExporter.Create(
                    manager,
                    "PlayerGirlRenameExporter"))
                {
                    if (!exporter.Initialize(temporaryPath, -1, manager.GetIOSettings()) ||
                        !exporter.Export(scene))
                    {
                        throw new InvalidOperationException(
                            "FBX SDK could not save renamed file: " + temporaryPath);
                    }
                }
            }
            if (renamedCount == 0 || !File.Exists(temporaryPath))
            {
                throw new InvalidOperationException(
                    "FBX core-bone rename did not modify the exported scene: " + outputPath);
            }
            File.Copy(temporaryPath, outputPath, true);
            File.Delete(temporaryPath);
            Debug.LogFormat(
                "PLAYERGIRL_FBX_CORE_BONES_RENAMED count={0} file={1}",
                renamedCount,
                outputPath);
        }

        private static int RewriteFbxNodeNames(
            Autodesk.Fbx.FbxNode node,
            IReadOnlyDictionary<string, string> exportedToDesiredName)
        {
            if (node == null)
            {
                return 0;
            }
            int renamedCount = 0;
            string name = node.GetName();
            if (exportedToDesiredName.TryGetValue(name, out string desiredName))
            {
                node.SetName(desiredName);
                ++renamedCount;
            }
            for (int index = 0; index < node.GetChildCount(); ++index)
            {
                renamedCount += RewriteFbxNodeNames(
                    node.GetChild(index),
                    exportedToDesiredName);
            }
            return renamedCount;
        }

        private static string ResolveOutputDirectory()
        {
            string value = Environment.GetEnvironmentVariable(
                "WW_AVATARGIRL_ORIGINAL_FBX_OUTPUT");
            if (!string.IsNullOrWhiteSpace(value))
            {
                return Path.GetFullPath(value);
            }
            return Path.GetFullPath(Path.Combine(Application.dataPath, "../MergedFBX"));
        }

        private static void EnsureTempFolder()
        {
            if (!AssetDatabase.IsValidFolder(TempRoot))
            {
                AssetDatabase.CreateFolder("Assets", "AvatarGirlOriginalLocomotionTemp");
            }
        }

        private static string SanitizeFileName(string value)
        {
            foreach (char invalid in Path.GetInvalidFileNameChars())
            {
                value = value.Replace(invalid, '_');
            }
            return value;
        }
    }
}
