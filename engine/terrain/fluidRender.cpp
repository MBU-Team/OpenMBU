//-----------------------------------------------------------------------------
// Torque Shader Engine
// Copyright (C) GarageGames.com, Inc.
//-----------------------------------------------------------------------------

#include "terrain/fluid.h"

//==============================================================================
//  FUNCTIONS
//==============================================================================

// MM:
//		All these nice neat transitional state blocks need blending together
//		to reduce the state changes.  I'll do this when I finally get to
//		locking it down. ;)
//
void fluid::Render(bool& EyeSubmerged)
{
    /*
        f32 BaseDriftX, BaseDriftY;

        f32 Q1 = 1.0f /   48.0f;    // This just looks good.
        f32 Q2 = 1.0f / 2048.0f;    // This is the size of the terrain.

        f32 SBase[] = { Q1,  0, 0, 0 };
        f32 TBase[] = { 0,  Q1, 0, 0 };

        f32 SLMap[] = { Q2,  0, 0, 0 };
        f32 TLMap[] = { 0,  Q2, 0, 0 };

        // Several attributes in the fluid vary over time.  Get a definitive time
        // reading now to be used throughout this render pass.
        m_Seconds = SECONDS - m_BaseSeconds;
        //m_BaseSeconds     = SECONDS;


        // Based on the view frustrum, accumulate the list of triangles that
        // comprise the fluid surface for this render pass.
        RunQuadTree( EyeSubmerged );

        CalcVertSpecular();

        // Quick debug render.
    #if 0
        if( 0 )
        {
            s32 i;
            for( i = 0; i < m_IUsed / 3; i++ )
            {
                glDisable   ( GL_TEXTURE_2D );
                glEnable    ( GL_BLEND );
                glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glBegin     ( GL_TRIANGLES );
                glColor4f   ( 0.0f, 1.0f, 0.0f, 0.5f );
                glVertex3f  ( m_pVertex[m_pIndex[i*3+0]].XYZ.X, m_pVertex[m_pIndex[i*3+0]].XYZ.Y, m_pVertex[m_pIndex[i*3+0]].XYZ.Z );
                glVertex3f  ( m_pVertex[m_pIndex[i*3+1]].XYZ.X, m_pVertex[m_pIndex[i*3+1]].XYZ.Y, m_pVertex[m_pIndex[i*3+1]].XYZ.Z );
                glVertex3f  ( m_pVertex[m_pIndex[i*3+2]].XYZ.X, m_pVertex[m_pIndex[i*3+2]].XYZ.Y, m_pVertex[m_pIndex[i*3+2]].XYZ.Z );
                glEnd       ();
            }
        }
    #endif

        //
        // We need to compute some time dependant values before we start rendering.
        //

        // Base texture drift.
        {
            // MM: Added Depth-Map Specific drift.
            if (m_UseDepthMap)
            {
                // MM: Adjust Flow Magnitude.
                m_FlowMagnitudeS = (m_FlowRate * m_Seconds) * mCos(mDegToRad(m_FlowAngle));
                m_FlowMagnitudeT = (m_FlowRate * m_Seconds) * mSin(mDegToRad(m_FlowAngle));

                // MM: Added Flow Control.
                BaseDriftX = m_FlowMagnitudeS;
                BaseDriftY = m_FlowMagnitudeT;
            }
            else
            {
                #define BASE_DRIFT_CYCLE_TIME    8.0f
                #define BASE_DRIFT_RATE          0.02f
                #define BASE_DRIFT_SCALAR        0.03f

                f32 Phase = FMOD( m_Seconds * (TWO_PI/BASE_DRIFT_CYCLE_TIME), TWO_PI );

                BaseDriftX = m_Seconds       * BASE_DRIFT_RATE;
                BaseDriftY = COSINE( Phase ) * BASE_DRIFT_SCALAR;
            }
        }


        //--------------------------------------------------------------------------
        //--
        //-- Let's rock.
        //--

        //--------------------------------------------------------------------------
        //-- Debug - wire

        if( m_ShowWire )
        {
            glPolygonMode       ( GL_FRONT_AND_BACK, GL_LINE );
            glEnableClientState ( GL_VERTEX_ARRAY );
            glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
            glDisable           ( GL_TEXTURE_2D );
            glDisable           ( GL_DEPTH_TEST );
            glEnable            ( GL_BLEND );
            glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glColor4f           ( 0.5f, 0.5f, 0.5f, 0.5f );
            glDrawElements      ( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
            glDisable           ( GL_BLEND );
            glEnable            ( GL_DEPTH_TEST );
            glColor4f           ( 1.0f, 1.0f, 0.0f, 1.0f );
            glDrawElements      ( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
            glPolygonMode       ( GL_FRONT_AND_BACK, GL_FILL );
        }

        // MM: Added completely new multi-texturing section.

        // Don't write into the Depth Buffer.
        glDepthMask	( GL_FALSE);

        // MM: Let's cull backfaces.
        //
        // NOTE:-	This will get promoted to an option for large-wave cases.
        glEnable(GL_CULL_FACE);

        // MM: Set culling mode.
        glCullFace(EyeSubmerged?GL_BACK:GL_FRONT);

        // MM: Select Appropriate Environment Map.
        U32 EnvMapViewTex = EyeSubmerged ? m_EnvMapUnderTexture.getGLName() : m_EnvMapOverTexture.getGLName();

        // Have we got multi-texturing?
        if (m_UseDepthMap && dglDoesSupportARBMultitexture())
        {
            // Yes, so have we got more than dual TMUs?
            if (dglGetMaxTextureUnits() > 2)
            {
                // More than Dual TMU's ... let's do it a lot quicker ...

                // *****************************************************************
                //
                //	Surface Map (Single-Pass).
                //
                // *****************************************************************

                // *****************************************************************
                // TMU #0 - Surface #1
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE0_ARB);
                glActiveTextureARB	(GL_TEXTURE0_ARB);
                glEnableClientState ( GL_VERTEX_ARRAY );
                glEnableClientState ( GL_TEXTURE_COORD_ARRAY );

                glBindTexture       ( GL_TEXTURE_2D, m_BaseTexture.getGLName() );
                glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
                glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV1) );
                glColor4f			( 1,1,1, m_Opacity );
                glEnable            ( GL_TEXTURE_2D );
                glEnable            ( GL_BLEND );
                glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glMatrixMode        ( GL_TEXTURE );
                glPushMatrix        ();
                glLoadIdentity      ();
                glTranslatef        ( BaseDriftX, BaseDriftY, 0.0f );

                // *****************************************************************
                // TMU #1 - Surface #2
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE1_ARB);
                glActiveTextureARB	(GL_TEXTURE1_ARB);
                glEnableClientState ( GL_VERTEX_ARRAY );
                glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
                glBindTexture       ( GL_TEXTURE_2D, m_BaseTexture.getGLName() );
                glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
                glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV1) );
                glColor4f			( 1,1,1, m_Opacity );
                glEnable            ( GL_TEXTURE_2D );
                glEnable            ( GL_BLEND );
                glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glMatrixMode        ( GL_TEXTURE );
                glPushMatrix        ();
                glLoadIdentity      ();
                glRotatef           ( 30.0f, 0.0f, 0.0f, 1.0f );
                glTranslatef        ( BaseDriftX * m_SurfaceParallax, BaseDriftY * m_SurfaceParallax, 0.0f );

                // *****************************************************************
                // TMU #2 - Alpha Depth Map.
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE2_ARB);
                glActiveTextureARB	(GL_TEXTURE2_ARB);
                glEnableClientState ( GL_VERTEX_ARRAY );
                glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
                glBindTexture       ( GL_TEXTURE_2D, m_DepthTexture.getGLName() );
                glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
                glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV4) );
                glEnable            ( GL_TEXTURE_2D );
                glEnable            ( GL_BLEND );
                glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                // *****************************************************************
                // Draw the Surface.
                // *****************************************************************
                if (dglDoesSupportCompiledVertexArray()) glLockArraysEXT(0, m_VUsed);
                glDrawElements( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
                if (dglDoesSupportCompiledVertexArray()) glUnlockArraysEXT();

                // *****************************************************************
                // Restore State #2 - Alpha Depth Map.
                // *****************************************************************
                glDisable			( GL_TEXTURE_2D );
                glDisable			( GL_BLEND );
                glTexEnvi			( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
                glDisableClientState( GL_VERTEX_ARRAY );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY );

                // *****************************************************************
                // Restore State #1 - Surface #2
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE1_ARB);
                glActiveTextureARB	(GL_TEXTURE1_ARB);
                glDisable			( GL_TEXTURE_2D );
                glDisable			( GL_BLEND );
                glTexEnvi			( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
                glDisableClientState( GL_VERTEX_ARRAY );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY  );
                glPopMatrix			();

                // *****************************************************************
                // Restore State #0 - Surface #1
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE0_ARB);
                glActiveTextureARB	(GL_TEXTURE0_ARB);
                glDisable			( GL_TEXTURE_2D );
                glDisable			( GL_BLEND );
                glTexEnvi			( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
                glDisableClientState( GL_VERTEX_ARRAY );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY  );
                glPopMatrix			();
            }
            else
            {
                // Dual TMU's ... let's do it a little slower ...

                // *****************************************************************
                //
                //	Surface Map (Double-Pass).
                //
                // *****************************************************************

                // *****************************************************************
                // TMU #0 - Surface #1
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE0_ARB);
                glActiveTextureARB	(GL_TEXTURE0_ARB);
                glEnableClientState ( GL_VERTEX_ARRAY );
                glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
                glBindTexture       ( GL_TEXTURE_2D, m_BaseTexture.getGLName() );
                glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
                glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV1) );
                glColor4f			( 1,1,1, m_Opacity );
                glEnable            ( GL_TEXTURE_2D );
                glEnable            ( GL_BLEND );
                glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glMatrixMode        ( GL_TEXTURE );
                glPushMatrix        ();
                glLoadIdentity      ();
                glTranslatef        ( BaseDriftX, BaseDriftY, 0.0f );

                // *****************************************************************
                // TMU #1 - Alpha Depth Map.
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE1_ARB);
                glActiveTextureARB	(GL_TEXTURE1_ARB);
                glEnableClientState ( GL_VERTEX_ARRAY );
                glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
                glBindTexture       ( GL_TEXTURE_2D, m_DepthTexture.getGLName() );
                glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
                glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV4) );
                glEnable            ( GL_TEXTURE_2D );
                glEnable            ( GL_BLEND );
                glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                // *****************************************************************
                // Draw the Surface#1 (Pass One)
                // *****************************************************************
                if (dglDoesSupportCompiledVertexArray()) glLockArraysEXT(0, m_VUsed);
                glDrawElements( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
                if (dglDoesSupportCompiledVertexArray()) glUnlockArraysEXT();

                // *****************************************************************
                // TMU #0 - Surface #2
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE0_ARB);
                glActiveTextureARB	(GL_TEXTURE0_ARB);

                glPopMatrix			();

                glEnableClientState ( GL_VERTEX_ARRAY );
                glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
                glBindTexture       ( GL_TEXTURE_2D, m_BaseTexture.getGLName() );
                glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
                glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV1) );
                glColor4f			( 1,1,1, m_Opacity );
                glEnable            ( GL_TEXTURE_2D );
                glEnable            ( GL_BLEND );
                glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glMatrixMode        ( GL_TEXTURE );
                glPushMatrix        ();
                glLoadIdentity      ();
                glRotatef           ( 30.0f, 0.0f, 0.0f, 1.0f );
                glTranslatef        ( BaseDriftX * m_SurfaceParallax, BaseDriftY * m_SurfaceParallax, 0.0f );

                // *****************************************************************
                // TMU #1 - Alpha Depth Map.
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE1_ARB);
                glActiveTextureARB	(GL_TEXTURE1_ARB);
                glEnableClientState ( GL_VERTEX_ARRAY );
                glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
                glBindTexture       ( GL_TEXTURE_2D, m_DepthTexture.getGLName() );
                glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
                glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV4) );
                glEnable            ( GL_TEXTURE_2D );
                glEnable            ( GL_BLEND );
                glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                // *****************************************************************
                // Draw the Surface#2 (Pass Two)
                // *****************************************************************
                if (dglDoesSupportCompiledVertexArray()) glLockArraysEXT(0, m_VUsed);
                glDrawElements( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
                if (dglDoesSupportCompiledVertexArray()) glUnlockArraysEXT();

                // *****************************************************************
                // Restore State #1 - Alpha Depth Map.
                // *****************************************************************
                glDisable			( GL_TEXTURE_2D );
                glDisable			( GL_BLEND );
                glTexEnvi			( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
                glDisableClientState( GL_VERTEX_ARRAY );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY  );

                // *****************************************************************
                // Restore State #0 - Surface #1/2
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE0_ARB);
                glActiveTextureARB	(GL_TEXTURE0_ARB);
                glDisable			( GL_TEXTURE_2D );
                glDisable			( GL_BLEND );
                glTexEnvi			( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY  );
                glDisableClientState( GL_VERTEX_ARRAY );
                glPopMatrix			();
            }


            // *****************************************************************
            //
            //	Shore Map.
            //
            // *****************************************************************


            // *****************************************************************
            // TMU #0 - Shore #1
            // *****************************************************************
            glClientActiveTextureARB(GL_TEXTURE0_ARB);
            glActiveTextureARB	(GL_TEXTURE0_ARB);
            glEnableClientState ( GL_VERTEX_ARRAY );
            glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
            glBindTexture       ( GL_TEXTURE_2D, m_ShoreTexture.getGLName() );
            glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
            glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV2) );
            glEnable            ( GL_TEXTURE_2D );
            glEnable            ( GL_BLEND );
            glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
            glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glMatrixMode        ( GL_TEXTURE );
            glPushMatrix        ();
            glLoadIdentity      ();
            glTranslatef        ( BaseDriftX, BaseDriftY, 0.0f );


            // *****************************************************************
            // TMU #1 - Shore Depth Map.
            // *****************************************************************
            glClientActiveTextureARB(GL_TEXTURE1_ARB);
            glActiveTextureARB	(GL_TEXTURE1_ARB);
            glEnableClientState ( GL_VERTEX_ARRAY );
            glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
            glBindTexture       ( GL_TEXTURE_2D, m_ShoreDepthTexture.getGLName() );
            glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
            glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV4) );
            glEnable            ( GL_TEXTURE_2D );
            glEnable            ( GL_BLEND );
            glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
            glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


            // *****************************************************************
            // Draw the Surface#1 (Pass One)
            // *****************************************************************
            if (dglDoesSupportCompiledVertexArray()) glLockArraysEXT(0, m_VUsed);
            glDrawElements( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
            if (dglDoesSupportCompiledVertexArray()) glUnlockArraysEXT();

            // *****************************************************************
            // Restore State #1 - Shore Depth Map.
            // *****************************************************************
            glDisable			( GL_TEXTURE_2D );
            glDisable			( GL_BLEND );
            glTexEnvi			( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
            glDisableClientState( GL_VERTEX_ARRAY );
            glDisableClientState( GL_TEXTURE_COORD_ARRAY  );

            // *****************************************************************
            // Restore State #0 - Shore #1
            // *****************************************************************
            glClientActiveTextureARB(GL_TEXTURE0_ARB);
            glActiveTextureARB	(GL_TEXTURE0_ARB);
            glDisable			( GL_TEXTURE_2D );
            glDisable			( GL_BLEND );
            glTexEnvi			( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
            glDisableClientState( GL_TEXTURE_COORD_ARRAY  );
            glDisableClientState( GL_VERTEX_ARRAY );
            glPopMatrix			();


            // *****************************************************************
          //
          // Specular highlights
          //
            // *****************************************************************
          glMatrixMode        ( GL_TEXTURE );
          glPushMatrix        ();
          glLoadIdentity      ();
          glRotatef           ( 30.0f, 0.0f, 0.0f, 1.0f );
          glScalef            ( 0.2, 0.2, 0.2 );

            glClientActiveTextureARB(GL_TEXTURE1_ARB);
          glDisable           ( GL_TEXTURE_2D );
            glClientActiveTextureARB(GL_TEXTURE0_ARB);
          glBindTexture       ( GL_TEXTURE_2D, m_SpecMaskTex.getGLName() );
          glEnable            ( GL_TEXTURE_2D );

            glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

          glEnable            ( GL_BLEND );
          glBlendFunc         ( GL_ONE, GL_ONE );

          glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
          glEnableClientState ( GL_VERTEX_ARRAY );
          glEnableClientState ( GL_COLOR_ARRAY );
          glColorPointer      ( 4, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].SPECULAR) );
          glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV1) );


          glDepthMask         ( GL_TRUE );
          glDrawElements      ( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );

          glDisableClientState( GL_VERTEX_ARRAY );
          glDisableClientState( GL_TEXTURE_COORD_ARRAY );
          glDisableClientState( GL_COLOR_ARRAY );

          glPopMatrix         ();
          glMatrixMode        ( GL_MODELVIEW );

            // *****************************************************************
            //
            //	Environment Map.
            //
            // *****************************************************************

            // Are we using the environment map?
            if (m_EnvMapIntensity > 0.0f)
            {
                // *****************************************************************
                // TMU #0 - Environment Map.
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE0_ARB);
                glActiveTextureARB(GL_TEXTURE0_ARB);
                glEnableClientState ( GL_VERTEX_ARRAY );
                glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
                glEnableClientState ( GL_COLOR_ARRAY );
                glBindTexture       ( GL_TEXTURE_2D, EnvMapViewTex );
                glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
                glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV3) );
                glColorPointer      ( 4, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].RGBA3) );
                glEnable            ( GL_TEXTURE_2D );
                glEnable            ( GL_BLEND );
                glBlendFunc         ( GL_SRC_ALPHA, GL_ONE );
                glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

                // *****************************************************************
                // TMU #1 - Alpha Depth Map.
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE1_ARB);
                glActiveTextureARB(GL_TEXTURE1_ARB);
                glEnableClientState ( GL_VERTEX_ARRAY );
                glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
                glBindTexture       ( GL_TEXTURE_2D, m_DepthTexture.getGLName() );
                glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
                glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV4) );
                glEnable            ( GL_TEXTURE_2D );
                glEnable            ( GL_BLEND );
                glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
                glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

                // *****************************************************************
                // Draw the Surface.
                // *****************************************************************
                if (dglDoesSupportCompiledVertexArray()) glLockArraysEXT(0, m_VUsed);
                glDrawElements  ( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
                if (dglDoesSupportCompiledVertexArray()) glUnlockArraysEXT();

                // *****************************************************************
                // Restore State #1 - Alpha Depth Map.
                // *****************************************************************
                glDisable			( GL_TEXTURE_2D );
                glDisable			( GL_BLEND );
                glTexEnvi			( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
                glDisableClientState( GL_COLOR_ARRAY  );
                glDisableClientState( GL_VERTEX_ARRAY );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY  );

                // *****************************************************************
                // Restore State #0 - ENvironment Map.
                // *****************************************************************
                glClientActiveTextureARB(GL_TEXTURE0_ARB);
                glActiveTextureARB	(GL_TEXTURE0_ARB);
                glDisable			( GL_TEXTURE_2D );
                glDisable			( GL_BLEND );
                glTexEnvi			( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
                glDisableClientState( GL_TEXTURE_COORD_ARRAY  );
                glDisableClientState( GL_VERTEX_ARRAY );
            }


            // *****************************************************************
            //
            // Fog Map.
            //
            // *****************************************************************

            // *****************************************************************
            // TMU #0 - Fog Map.
            // *****************************************************************
            glEnableClientState ( GL_VERTEX_ARRAY );
            glEnableClientState ( GL_COLOR_ARRAY );
            glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );
            glColorPointer      ( 4, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].RGBA4) );
            glEnable            ( GL_BLEND );
            glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            // *****************************************************************
            // Draw the Surface.
            // *****************************************************************
            if (dglDoesSupportCompiledVertexArray()) glLockArraysEXT(0, m_VUsed);
            glDrawElements  ( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
            if (dglDoesSupportCompiledVertexArray()) glUnlockArraysEXT();

            // *****************************************************************
            // Restore State #0 - Fog Map.
            // *****************************************************************
            glDisable            ( GL_BLEND );
            glDisableClientState ( GL_COLOR_ARRAY  );
            glDisableClientState ( GL_VERTEX_ARRAY );

            // Restore Current Matrix.
            glMatrixMode        ( GL_MODELVIEW );
        }
        else
        {
            //--------------------------------------------------------------------------
            //-- Initializations for Phase 1 - base textures

            glPolygonMode       ( GL_FRONT_AND_BACK, GL_FILL );

            glEnableClientState ( GL_VERTEX_ARRAY );
            glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );

            glEnable            ( GL_TEXTURE_2D );
            glEnable            ( GL_BLEND );

            glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
            glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            glMatrixMode        ( GL_TEXTURE );
            glPushMatrix        ();
            glLoadIdentity      ();

    //		glEnableClientState ( GL_COLOR_ARRAY );													// MM: Removed Colour Array.

            glEnable            ( GL_TEXTURE_GEN_S );
            glEnable            ( GL_TEXTURE_GEN_T );
            glTexGeni           ( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
            glTexGeni           ( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
            glTexGenfv          ( GL_S, GL_OBJECT_PLANE, SBase );
            glTexGenfv          ( GL_T, GL_OBJECT_PLANE, TBase );

            glBindTexture       ( GL_TEXTURE_2D, m_BaseTexture.getGLName() );

            //--------------------------------------------------------------------------
            //-- Initializations for Phase 1a - first base texture

            glTranslatef        ( BaseDriftX, BaseDriftY, 0.0f );									// MM: Added Drift Translation.
            //glRotatef           ( 30.0f, 0.0f, 0.0f, 1.0f );										// MM: Removed Rotation.
    //		glColorPointer      ( 4, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].RGBA1a) );			// MM: Removed Colour Array.
            glColor4f			( 1,1,1, m_Opacity );												// MM: Added Opacity.




            //--------------------------------------------------------------------------
            //-- Render Phase 1a - first base texture

            if( m_ShowBaseA )
            {
                glDrawElements  ( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
            }

            //--------------------------------------------------------------------------
            //-- Initializations for Phase 1b - second base texture

            glRotatef           ( 30.0f, 0.0f, 0.0f, 1.0f );
            glTranslatef        ( BaseDriftX * m_SurfaceParallax, BaseDriftY * m_SurfaceParallax, 0.0f );	// MM: Added Drift Translation.
    //		glColorPointer      ( 4, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].RGBA1b) );					// MM: Removed Colour Array.

            //--------------------------------------------------------------------------
            //-- Render Phase 1b - first base texture

            if( m_ShowBaseB )
            {
                glDrawElements  ( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
            }

            //--------------------------------------------------------------------------
            //-- Cleanup from Phase 1 - base textures

            glDisableClientState( GL_COLOR_ARRAY  );												// MM: Removed Colour Array.
            glPopMatrix         ();

            //--------------------------------------------------------------------------
            //-- Initializations for Phase 2 - light map

            // glColor4f           ( 1.0f, 1.0f, 1.0f, 1.0f );
            // glBindTexture       ( GL_TEXTURE_2D, m_LightMapTexture.getGLName() );

            // glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
            // glBlendFunc         ( GL_DST_COLOR, GL_ZERO );

            // glMatrixMode        ( GL_TEXTURE );
            // glPushMatrix        ();
            // glLoadIdentity      ();

            // glTexGenfv          ( GL_S, GL_OBJECT_PLANE, SLMap );
            // glTexGenfv          ( GL_T, GL_OBJECT_PLANE, TLMap );

            //--------------------------------------------------------------------------
            //-- Render Phase 2 - light map

            // if( m_ShowLightMap )
            // {
                // glDrawElements  ( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
            // }

            //--------------------------------------------------------------------------
            //-- Cleanup from Phase 2 - light map

            // glPopMatrix         ();
            glMatrixMode        ( GL_MODELVIEW );

            glDisable           ( GL_TEXTURE_GEN_S );
            glDisable           ( GL_TEXTURE_GEN_T );

            glEnableClientState ( GL_VERTEX_ARRAY );
            glVertexPointer     ( 3, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].XYZ) );

            glEnable            ( GL_TEXTURE_2D );
            glEnable            ( GL_BLEND );

            //--------------------------------------------------------------------------
            //-- Initializations for Phase 3 - environment map

            glEnableClientState ( GL_TEXTURE_COORD_ARRAY );
            glTexCoordPointer   ( 2, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].UV3) );

            glTexEnvi           ( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
            glBlendFunc         ( GL_SRC_ALPHA, GL_ONE );

            glColor4f           ( 1.0f, 1.0f, 1.0f, m_EnvMapIntensity );
            glBindTexture       ( GL_TEXTURE_2D, EnvMapViewTex );

            //--------------------------------------------------------------------------
            //-- Render Phase 3 - environment map / specular

            if( m_ShowEnvMap )
            {
                glDrawElements  ( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
            }

            //--------------------------------------------------------------------------
            //-- Initializations for Phase 4 - fog

            glDisable           ( GL_TEXTURE_2D );
            glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            glEnableClientState ( GL_COLOR_ARRAY );
            glColorPointer      ( 4, GL_FLOAT, sizeof(vertex), &(m_pVertex[0].RGBA4) );

            glDisableClientState( GL_TEXTURE_COORD_ARRAY );

            //--------------------------------------------------------------------------
            //-- Render Phase 4 - fog

            if( m_ShowFog )
            {
                glDrawElements  ( GL_TRIANGLES, m_IUsed, GL_UNSIGNED_SHORT, m_pIndex );
            }
        }



        //--------------------------------------------------------------------------
        //-- Cleanup from all Phases

        // MM: Restore culling mode.
        glCullFace(GL_BACK);
        // MM: Stop culling backfaces.
        glDisable(GL_CULL_FACE);

        // Continue writing into the Depth Buffer.
        glDepthMask(GL_TRUE);

        glDisable           ( GL_BLEND );
        glDisableClientState( GL_VERTEX_ARRAY );
        glDisableClientState( GL_COLOR_ARRAY  );

        //--------------------------------------------------------------------------
        //-- We're done with the rendering.
    */
}

//==============================================================================
void fluid::CalcVertSpecular()
{
    // get sun direction
    Point3F lightDir(0.5, 0.5, -0.5);
    lightDir.normalize();

    // get eye position
    Point3F eyePos(m_Eye.X, m_Eye.Y, m_Eye.Z);

    // loop through all displayed verts
    for (U32 i = 0; i < m_IUsed; i++)
    {
        vertex* vert = m_pVertex + m_pIndex[i];
        Point3F vertPos(vert->XYZ.X, vert->XYZ.Y, vert->XYZ.Z);

        // perform half angle calculation
        Point3F eyeVec = eyePos - vertPos;
        eyeVec.normalize();
        Point3F half = -lightDir + eyeVec;
        half.normalize();
        F32 specular = mDot(half, Point3F(0.0, 0.0, 1.0));
        if (specular < 0.0) specular = 0.0;

        specular = pow(specular, m_SpecPower);

        // store specular color in vert
        vert->SPECULAR = m_SpecColor * specular;
    }
}

//==============================================================================
